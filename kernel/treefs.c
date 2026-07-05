#include "treefs.h"
#include "memory.h" 
#include "uart.h"

/* Variaveis Globais (Disco Virtual na memoria ram)*/
superblock_t *sb;
uint8_t *inode_bitmap;
uint8_t *block_bitmap;
inode_t *inode_table;
uint8_t *data_blocks;

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

    // Criando a estrutura inicial obrigatoria /home, /tmp, /bin 
    const char* default_dirs[] = {"home", "tmp", "bin"};
    
    for(int i = 0; i < 3; i++) {
        uint32_t dir_ino = inode_alloc();
        inode_table[dir_ino].type = TYPE_DIR;
        inode_table[dir_ino].size = 0;
        inode_table[dir_ino].ref_count = 1;
        
        // Adiciona a pasta arrecem criada dentro da raiz
        add_dir_entry(root_ino, default_dirs[i], dir_ino);
    }

    uart_print("[TreeFS] Sistema de arquivos inicializado com sucesso!\n");
    return 0;
}