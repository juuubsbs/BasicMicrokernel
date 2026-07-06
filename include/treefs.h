#pragma once
#include <stdint.h>

#define BLOCK_SIZE 512
#define MAX_INODES 128
#define MAX_BLOCKS 256
#define MAX_FILENAME 32

/* Tipos de Inode */
#define TYPE_FILE 1
#define TYPE_DIR  2

/* Permissoes (bonus): rwx simplificado, sem distincao de usuario/grupo */
#define PERM_READ  0x4
#define PERM_WRITE 0x2
#define PERM_EXEC  0x1
#define PERM_DEFAULT_FILE (PERM_READ | PERM_WRITE)
#define PERM_DEFAULT_DIR  (PERM_READ | PERM_WRITE | PERM_EXEC)

/* Superblock */
typedef struct {
    uint32_t magic;         // Assinatura do sistema (ex: 0x54524545 para "TREE")
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t block_size;
} superblock_t;

/* Inode */
typedef struct {
    uint32_t type;          // TYPE_FILE ou TYPE_DIR
    uint32_t size;          // Tamanho em bytes
    uint32_t ref_count;
    uint32_t perm;          // Permissoes rwx simplificadas (bonus)
    uint32_t blocks[8];     // Ponteiros diretos para os blocos
    uint32_t indirect;      // Bloco de indices (bonus: blocos indiretos)
    uint64_t created_at;    // Timestamp de criacao (bonus)
    uint64_t modified_at;   // Timestamp da ultima escrita (bonus)
} inode_t;

/* Entrada de Diretório (Associa um nome a um inode) */
typedef struct {
    char name[MAX_FILENAME];
    uint32_t inode_num;
} dirent_t;

/* API */
int fs_init(void);
inode_t *path_lookup(const char *path);
int mkdir(const char *path);
int create(const char *path);
int ls(const char *path);
int unlink(const char *path);
int read(int fd, void *buf, uint32_t size);
int write(int fd, const void *buf, uint32_t size);

/* API extra (bonus) */
int fs_stat(const char *path); // imprime metadados do inode (tipo, tamanho, links, timestamps)
int open(const char *path);                          // fd == numero do inode
int link(const char *oldpath, const char *newpath);   // link fisico (hard link)
int chmod(const char *path, uint32_t mode);           // altera permissoes (PERM_READ|PERM_WRITE|PERM_EXEC)