#pragma once
#include <stdint.h>

#define MAX_TASKS 8
#define DEFAULT_STACK_SIZE 2048

// Estados possíveis para o ciclo de vida da Task (Essencial para o Scheduler Dinâmico)
typedef enum {
    TASK_FREE = 0,     // Slot vazio/disponível ou tarefa já destruída
    TASK_READY,        // Tarefa pronta para executar, aguardando sua vez
    TASK_RUNNING       // Tarefa atualmente em execução na CPU
} task_state_t;

typedef struct
{
    uint64_t regs[31];   // x1–x31 (x0 é zero)
    void (*entry)(void);
    int priority;
    void *stack;
    task_state_t state;

} TCB;

extern TCB tasks[MAX_TASKS];
extern int task_count;

void xTaskCreate(void (*task)(void),
                 uint32_t stack_size,
                 int priority);

void xTaskDelete(int task_index);
