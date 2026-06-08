#include "task.h"
#include "memory.h"

TCB tasks[MAX_TASKS];
int task_count = 0;

void xTaskCreate(void (*task)(void), uint32_t stack_size, int priority)
{
    if (task_count >= MAX_TASKS) return;
    if (stack_size == 0) stack_size = DEFAULT_STACK_SIZE;

    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].entry == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return;

    TCB *t = &tasks[slot];

    t->stack = (uint8_t*)kmalloc(stack_size);
    if (!t->stack) return;

    uint64_t *sp = (uint64_t*)(t->stack + stack_size);
    sp = (uint64_t*)((uint64_t)sp & -16LL); // Alinhamento da ABI do RISC-V

    // Limpa o contexto
    for(int i = 0; i < 32; i++) t->regs[i] = 0;

    // Configuração idêntica à ordem do seu context.S:
    t->regs[0] = (uint64_t)task;   // offset 0  -> ra
    t->regs[1] = (uint64_t)sp;     // offset 8  -> sp

    t->entry = task;
    t->state = TASK_READY;
    t->priority = priority;

    task_count++;
}

void xTaskDelete(int task_index)
{
    if (task_index < 0 || task_index >= MAX_TASKS) return;

    TCB *t = &tasks[task_index];

    if (t->stack) {
        kfree(t->stack);
        t->stack = 0;
    }

    t->entry = 0;
    t->priority = 0;
}