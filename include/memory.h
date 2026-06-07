#pragma once
#include <stdint.h>

/* Inicializa o heap */
void memory_init(void);

/* Alocacao dinamica via free list */
void *kmalloc(uint64_t size);

/* Liberacao basica de bloco */
void kfree(void *ptr);

/* Informacoes */
uint64_t memory_used(void);
uint64_t memory_free(void);
uint64_t memory_total(void);
uint64_t memory_fragmentation(void);
void memory_dump(void);