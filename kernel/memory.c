#include "memory.h"
#include "uart.h"

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

// alocação em múltiplos de 8, arredonda pra cima  
static uint64_t align8(uint64_t size)
{
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

// Junta blocos livres adjacentes para evitar fragmentação externa
static void coalesce_blocks(void)
{
    block_t *current = free_list;
    while (current && current->next)
    {
        if (current->free && current->next->free)
        {
            // O tamanho do novo bloco unido absorve o tamanho do próximo + o cabeçalho dele
            current->size += sizeof(block_t) + current->next->size;
            current->next = current->next->next; // Pula o bloco fundido
            // Não avança o 'current' pois o novo próximo também pode estar livre!
        }
        else
        {
            current = current->next;
        }
    }
}

static void split_block(block_t *block, uint64_t size)
{
    // O espaço restante precisa abrigar pelo menos um novo cabeçalho + alinhamento mínimo
    if (block->size <= size + sizeof(block_t) + ALIGNMENT)
        return;

    uint64_t remaining = block->size - size - sizeof(block_t);

    // Cria o novo cabeçalho deslocado na memória RAM
    block_t *next = (block_t *)((uint8_t *)(block + 1) + size);
    next->size = remaining;
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

void kfree(void *ptr)
{
    if (!ptr)
        return;

    block_t *block = ((block_t *)ptr) - 1;
    block->free = 1;

    coalesce_blocks();
}

/* Estatisticas */

uint64_t memory_used(void)
{
    block_t *current = free_list;
    uint64_t used = 0;

    while (current)
    {
        if (!current->free)
            used += current->size + sizeof(block_t);

        current = current->next;
    }

    return used;
}

uint64_t memory_free(void){

    return HEAP_SIZE - memory_used();
}

uint64_t memory_total(void)
{
    return HEAP_SIZE;
}

uint64_t memory_fragmentation(void)
{
    block_t *current = free_list;
    uint64_t total_free_usable = 0;
    uint64_t max_free_block = 0;

    // Varre a lista calculando os valores baseados estritamente nos blocos livres
    while (current)
    {
        if (current->free)
        {
            total_free_usable += current->size;
            if (current->size > max_free_block)
            {
                max_free_block = current->size;
            }
        }
        current = current->next;
    }

    // Se não há memória livre de verdade, a fragmentação é zero
    if (total_free_usable == 0) return 0;

    // Se o maior bloco livre é exatamente igual à soma de todos os espaços livres,
    // significa que a memória está contígua (0% de fragmentação)
    uint64_t frag = 100 - ((max_free_block * 100) / total_free_usable);
    return frag;
}

void memory_dump(void)
{
    block_t *current = free_list;
    int bloco_id = 1;

    uart_print("\n===== MAPA REAL DO HEAP (FREE LIST) =====\n");
    while (current)
    {
        uart_print("[Bloco ");
        uart_print_uint(bloco_id++);
        uart_print("] End: 0x");
        // Imprime o endereço do bloco (convertendo o ponteiro para uint64)
        uart_print_uint((uint64_t)current); 
        uart_print(" | Tam: ");
        uart_print_uint(current->size);
        uart_print(" bytes | Status: ");
        
        if (current->free) {
            uart_print("LIVRE [ . ]\n");
        } else {
            uart_print("OCUPADO [ X ]\n");
        }

        current = current->next;
    }
    uart_print("=========================================\n\n");
}


