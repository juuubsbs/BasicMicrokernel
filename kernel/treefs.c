#include "treefs.h"
#include "memory.h"
#include "uart.h"
#include "string.h"

/* Variaveis Globais (Disco Virtual na memoria ram)*/
superblock_t *sb;
uint8_t *inode_bitmap;
uint8_t *block_bitmap;
inode_t *inode_table;
uint8_t *data_blocks;

/* Inode da raiz "/", guardado para path_lookup() nao depender de indice fixo */
static uint32_t g_root_ino;

/* Macros para manipular Bitmaps (Arrays de bits)
Operações bit a bit (bitwise) em arrays de uint8_t para marcar se um inode/bloco está livre (0) ou ocupado (1)*/
#define SET_BIT(bitmap, i)   (bitmap[(i) / 8] |=  (1 << ((i) % 8)))
#define CLEAR_BIT(bitmap, i) (bitmap[(i) / 8] &= ~(1 << ((i) % 8)))
#define TEST_BIT(bitmap, i)  (bitmap[(i) / 8] &   (1 << ((i) % 8)))

/* GERENCIAMENTO DE INODES E BLOCOS*/

uint32_t inode_alloc(void) {
    for (uint32_t i = 0; i < MAX_INODES; i++) {
        if (!TEST_BIT(inode_bitmap, i)) { // Se o bit é 0, está livre 
            SET_BIT(inode_bitmap, i);     // Marca como ocupado 
            return i;
        }
    }
    return -1; // Esgotamos os inodes
}

void inode_free(uint32_t inode_num) {
    if (inode_num < MAX_INODES) {
        CLEAR_BIT(inode_bitmap, inode_num); // Libera o inode 
    }
}

int block_alloc(void) {
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        if (!TEST_BIT(block_bitmap, i)) { // Localiza bloco livre 
            SET_BIT(block_bitmap, i);     // Reserva bloco 
            return i;
        }
    }
    return -1; // Esgotamos os blocos de dados
}

void block_free(uint32_t block_num) {
    if (block_num < MAX_BLOCKS) {
        CLEAR_BIT(block_bitmap, block_num); // Libera o bloco 
    }
}

/* INICIALIZAÇÃO DO SISTEMA DE ARQUIVOS (FORMAT) */

/* Função auxiliar interna para escrever uma entrada dentro de um diretório */
static void add_dir_entry(uint32_t parent_ino, const char *name, uint32_t child_ino) {
    inode_t *parent = &inode_table[parent_ino];
    
    // Se o diretório pai ainda não tem nenhum bloco de dados, alocamos o primeiro
    if (parent->size == 0) {
        parent->blocks[0] = block_alloc();
    }
    
    // Endereço do bloco do diretorio
    uint32_t block_idx = parent->blocks[0];
    dirent_t *entries = (dirent_t *)(data_blocks + (block_idx * BLOCK_SIZE));
    // Verificamos quantas entradas já existem baseadas no tamanho do diretório
    int num_entries = parent->size / sizeof(dirent_t);
    // Simula um strcpy
    int i;
    for(i = 0; name[i] != '\0' && i < MAX_FILENAME - 1; i++) {
        entries[num_entries].name[i] = name[i];
    }
    entries[num_entries].name[i] = '\0';
    
    // Salvamos qual é o inode daquele arquivo
    entries[num_entries].inode_num = child_ino;
    // Aumentamos o tamanho do diretorio pai
    parent->size += sizeof(dirent_t);
}

int fs_init(void) {
    uart_print("\n[TreeFS] Formatando o disco virtual...\n");

    // Alocando as areas do disco 
    sb = (superblock_t *)kmalloc(sizeof(superblock_t));
    inode_bitmap = (uint8_t *)kmalloc(MAX_INODES / 8);
    block_bitmap = (uint8_t *)kmalloc(MAX_BLOCKS / 8);
    inode_table = (inode_t *)kmalloc(MAX_INODES * sizeof(inode_t));
    data_blocks = (uint8_t *)kmalloc(MAX_BLOCKS * BLOCK_SIZE);

    // Zerando os bitmaps para garantir que comece tudo livre
    for(int i = 0; i < MAX_INODES / 8; i++) inode_bitmap[i] = 0;
    for(int i = 0; i < MAX_BLOCKS / 8; i++) block_bitmap[i] = 0;

    // Preenchendo o Superblock 
    sb->magic = 0x54524545; // Assinatura "TREE" em hexadecimal
    sb->total_blocks = MAX_BLOCKS;
    sb->total_inodes = MAX_INODES;
    sb->block_size = BLOCK_SIZE;
    SET_BIT(block_bitmap, 0);

    // Criando a Raiz "/" (Inode 0)
    uint32_t root_ino = inode_alloc();
    inode_table[root_ino].type = TYPE_DIR;
    inode_table[root_ino].size = 0;
    inode_table[root_ino].ref_count = 1;
    for (int i = 0; i < 8; i++) inode_table[root_ino].blocks[i] = 0;
    g_root_ino = root_ino;

    // Criando a estrutura inicial obrigatoria /home, /tmp, /bin
    const char* default_dirs[] = {"home", "tmp", "bin"};

    for(int i = 0; i < 3; i++) {
        uint32_t dir_ino = inode_alloc();
        inode_table[dir_ino].type = TYPE_DIR;
        inode_table[dir_ino].size = 0;
        inode_table[dir_ino].ref_count = 1;
        for (int j = 0; j < 8; j++) inode_table[dir_ino].blocks[j] = 0;

        // Adiciona a pasta arrecem criada dentro da raiz
        add_dir_entry(root_ino, default_dirs[i], dir_ino);
    }

    uart_print("[TreeFS] Sistema de arquivos inicializado com sucesso!\n");
    return 0;
}

/* RESOLUCAO DE CAMINHOS */

/* Procura "name" entre as entradas do diretorio dir_ino. Retorna o
 * numero do inode associado ou -1 se nao encontrar. */
static int find_entry(uint32_t dir_ino, const char *name) {
    inode_t *dir = &inode_table[dir_ino];
    if (dir->size == 0) return -1;

    uint32_t block_idx = dir->blocks[0];
    dirent_t *entries = (dirent_t *)(data_blocks + (block_idx * BLOCK_SIZE));
    int num_entries = dir->size / sizeof(dirent_t);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            return (int)entries[i].inode_num;
        }
    }
    return -1;
}

/* Avanca "path" para o proximo componente (entre barras), copiando-o
 * em "comp". Retorna 0 quando o caminho acabou. */
static int next_component(const char **path, char *comp) {
    while (**path == '/') (*path)++;

    if (**path == '\0') {
        comp[0] = '\0';
        return 0;
    }

    int i = 0;
    while (**path != '/' && **path != '\0' && i < MAX_FILENAME - 1) {
        comp[i++] = *(*path)++;
    }
    comp[i] = '\0';
    return 1;
}

/* Percorre a arvore a partir da raiz, componente por componente, ate
 * localizar o inode correspondente ao caminho absoluto informado. */
inode_t *path_lookup(const char *path) {
    if (!path || path[0] != '/') return 0;

    uint32_t current = g_root_ino;

    if (path[1] == '\0') {
        return &inode_table[current];
    }

    const char *p = path;
    char comp[MAX_FILENAME];

    while (next_component(&p, comp)) {
        if (inode_table[current].type != TYPE_DIR) return 0;

        int found = find_entry(current, comp);
        if (found < 0) return 0;

        current = (uint32_t)found;
    }

    return &inode_table[current];
}

/* Localiza o diretorio pai de "path" e extrai o ultimo componente
 * (nome do arquivo/diretorio a ser criado ou removido). */
static int split_parent(const char *path, uint32_t *parent_out, char *name_out) {
    if (!path || path[0] != '/') return -1;

    int len = (int)strlen(path);
    int last_slash = -1;
    for (int i = 0; i < len; i++) {
        if (path[i] == '/') last_slash = i;
    }
    if (last_slash < 0) return -1;

    strcpy(name_out, path + last_slash + 1);
    if (name_out[0] == '\0') return -1; // caminho termina em "/", sem nome

    char parent_path[128];
    if (last_slash == 0) {
        parent_path[0] = '/';
        parent_path[1] = '\0';
    } else {
        memcpy(parent_path, path, last_slash);
        parent_path[last_slash] = '\0';
    }

    inode_t *parent = path_lookup(parent_path);
    if (!parent || parent->type != TYPE_DIR) return -1;

    *parent_out = (uint32_t)(parent - inode_table);
    return 0;
}

/* NAVEGACAO */

int ls(const char *path) {
    inode_t *dir = path_lookup(path);
    if (!dir || dir->type != TYPE_DIR) return -1;

    if (dir->size == 0) return 0;

    uint32_t block_idx = dir->blocks[0];
    dirent_t *entries = (dirent_t *)(data_blocks + (block_idx * BLOCK_SIZE));
    int num_entries = dir->size / sizeof(dirent_t);

    for (int i = 0; i < num_entries; i++) {
        uart_print(entries[i].name);
        uart_print("\n");
    }
    return 0;
}

/* CRIACAO DE DIRETORIOS E ARQUIVOS */

int mkdir(const char *path) {
    uint32_t parent_ino;
    char name[MAX_FILENAME];

    if (split_parent(path, &parent_ino, name) < 0) return -1;
    if (find_entry(parent_ino, name) >= 0) return -1; // duplicidade

    uint32_t new_ino = inode_alloc();
    if (new_ino == (uint32_t)-1) return -1;

    inode_table[new_ino].type = TYPE_DIR;
    inode_table[new_ino].size = 0;
    inode_table[new_ino].ref_count = 1;
    for (int i = 0; i < 8; i++) inode_table[new_ino].blocks[i] = 0;

    add_dir_entry(parent_ino, name, new_ino);
    return 0;
}

int create(const char *path) {
    uint32_t parent_ino;
    char name[MAX_FILENAME];

    if (split_parent(path, &parent_ino, name) < 0) return -1;
    if (find_entry(parent_ino, name) >= 0) return -1; // duplicidade

    uint32_t new_ino = inode_alloc();
    if (new_ino == (uint32_t)-1) return -1;

    inode_table[new_ino].type = TYPE_FILE;
    inode_table[new_ino].size = 0;
    inode_table[new_ino].ref_count = 1;
    for (int i = 0; i < 8; i++) inode_table[new_ino].blocks[i] = 0;

    add_dir_entry(parent_ino, name, new_ino);

    // fd == numero do inode: nao ha open(), entao create() ja devolve
    // o identificador usado por read()/write().
    return (int)new_ino;
}

/* ESCRITA E LEITURA DE ARQUIVOS */

int write(int fd, const void *buf, uint32_t size) {
    if (fd < 0 || fd >= MAX_INODES) return -1;

    inode_t *inode = &inode_table[fd];
    if (inode->type != TYPE_FILE) return -1;

    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (blocks_needed > 8) return -1; // apenas blocos diretos (sem indirecao)

    const uint8_t *src = (const uint8_t *)buf;
    uint32_t remaining = size;

    for (uint32_t i = 0; i < blocks_needed; i++) {
        int blk;
        if (inode->blocks[i] != 0) {
            blk = (int)inode->blocks[i]; // reaproveita bloco ja alocado
        } else {
            blk = block_alloc();
            if (blk < 0) return -1;
            inode->blocks[i] = (uint32_t)blk;
        }

        uint32_t chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
        memcpy(data_blocks + ((uint32_t)blk * BLOCK_SIZE), src, chunk);

        src += chunk;
        remaining -= chunk;
    }

    inode->size = size;
    return (int)size;
}

int read(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= MAX_INODES) return -1;

    inode_t *inode = &inode_table[fd];
    if (inode->type != TYPE_FILE) return -1;

    uint32_t to_read = size < inode->size ? size : inode->size;
    uint8_t *dst = (uint8_t *)buf;
    uint32_t remaining = to_read;
    uint32_t i = 0;

    while (remaining > 0) {
        uint32_t blk = inode->blocks[i];
        uint32_t chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;

        memcpy(dst, data_blocks + (blk * BLOCK_SIZE), chunk);

        dst += chunk;
        remaining -= chunk;
        i++;
    }

    return (int)to_read;
}

/* REMOCAO DE ARQUIVOS */

/* Remove a entrada "name" do diretorio pai, deslocando as entradas
 * seguintes para preencher o espaco (mantendo o bloco compacto). */
static void remove_dir_entry(uint32_t parent_ino, const char *name) {
    inode_t *parent = &inode_table[parent_ino];
    if (parent->size == 0) return;

    uint32_t block_idx = parent->blocks[0];
    dirent_t *entries = (dirent_t *)(data_blocks + (block_idx * BLOCK_SIZE));
    int num_entries = parent->size / sizeof(dirent_t);

    int found = -1;
    for (int i = 0; i < num_entries; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            found = i;
            break;
        }
    }
    if (found < 0) return;

    for (int i = found; i < num_entries - 1; i++) {
        entries[i] = entries[i + 1];
    }
    parent->size -= sizeof(dirent_t);
}

int unlink(const char *path) {
    uint32_t parent_ino;
    char name[MAX_FILENAME];

    if (split_parent(path, &parent_ino, name) < 0) return -1;

    int ino = find_entry(parent_ino, name);
    if (ino < 0) return -1;

    inode_t *inode = &inode_table[ino];
    if (inode->type != TYPE_FILE) return -1; // remocao de diretorios nao suportada (rmdir)

    uint32_t nblocks = (inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint32_t i = 0; i < nblocks; i++) {
        block_free(inode->blocks[i]);
        inode->blocks[i] = 0;
    }

    inode_free((uint32_t)ino);
    inode->size = 0;
    inode->type = 0;

    remove_dir_entry(parent_ino, name);
    return 0;
}