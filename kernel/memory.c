#include "memory.h"

/* Configuracao do heap conforme a proposta da M2: 64 KiB. */

#define HEAP_SIZE 0x10000
#define ALIGNMENT 8

typedef struct block
{
    uint64_t size;
    int free;
    struct block *next;
} block_t;

static uint8_t heap[HEAP_SIZE] __attribute__((aligned(ALIGNMENT)));
static block_t *free_list;

static uint64_t align8(uint64_t size)
{
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static void split_block(block_t *block, uint64_t size)
{
    uint64_t remaining = block->size - size;

    if (remaining <= sizeof(block_t) + ALIGNMENT)
        return;

    block_t *next = (block_t *)((uint8_t *)(block + 1) + size);
    next->size = remaining - sizeof(block_t);
    next->free = 1;
    next->next = block->next;

    block->size = size;
    block->next = next;
}

/* Inicializacao */

void memory_init(void)
{
    free_list = (block_t *)heap;
    free_list->size = HEAP_SIZE - sizeof(block_t);
    free_list->free = 1;
    free_list->next = 0;
}

/* Alocador First Fit com divisao de blocos. */

void *kmalloc(uint64_t size)
{
    if (size == 0)
        return 0;

    size = align8(size);

    block_t *current = free_list;

    while (current)
    {
        if (current->free && current->size >= size)
        {
            split_block(current, size);
            current->free = 0;

            return (void *)(current + 1);
        }

        current = current->next;
    }

    return 0;
}

/* Liberacao basica: a coalescencia fica para a proxima etapa. */

void kfree(void *ptr)
{
    if (!ptr)
        return;

    block_t *block = ((block_t *)ptr) - 1;
    block->free = 1;
}

/* Estatisticas */

uint64_t memory_used(void)
{
    block_t *current = free_list;
    uint64_t used = 0;

    while (current)
    {
        if (!current->free)
            used += current->size;

        current = current->next;
    }

    return used;
}

uint64_t memory_free(void)
{
    block_t *current = free_list;
    uint64_t free_mem = 0;

    while (current)
    {
        if (current->free)
            free_mem += current->size;

        current = current->next;
    }

    return free_mem;
}

uint64_t memory_total(void)
{
    return HEAP_SIZE;
}
