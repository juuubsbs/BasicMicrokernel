#include "scheduler.h"
#include "task.h"

extern void context_switch(void*, void*);
static int current = 0;

static int round_robin()
{
    int next = current;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        next = (next + 1) % MAX_TASKS;
        if (tasks[next].entry != 0 && (tasks[next].state == TASK_READY || tasks[next].state == TASK_RUNNING)) {
            return next;
        }
    }
    return current;
}

static sched_algo_t current_algo = round_robin;

void scheduler_set_algorithm(sched_algo_t algo)
{
    if (algo) current_algo = algo;
}

void yield()
{
    int prev = current;
    int next = current_algo();

    if (prev != next) 
    {
        tasks[prev].state = TASK_READY;
        current = next;
        tasks[next].state = TASK_RUNNING;
        context_switch(tasks[prev].regs, tasks[next].regs);
    }
}

void schedule_from_trap(uint64_t *frame)
{
    int prev = current;
    int next = current_algo();

    if (prev != next)
    {
        /* Mapeamento Manual: Salva o frame do trap_entry.S 
           dentro do vetor regs[] respeitando a ordem do seu context.S
        */
        tasks[prev].regs[0]  = frame[0];   // ra
        tasks[prev].regs[1]  = frame[30];  // sp (No seu trap_entry, o SP antigo está no offset 240/8 = 30)
        tasks[prev].regs[2]  = frame[1];   // gp
        tasks[prev].regs[3]  = frame[2];   // tp
        
        // Temporários t0 - t2
        for (int i = 0; i < 3; i++) tasks[prev].regs[4 + i] = frame[3 + i];
        
        // Salvos s0 - s1
        tasks[prev].regs[7]  = frame[6];   // s0
        tasks[prev].regs[8]  = frame[7];   // s1
        
        // Argumentos a0 - a7
        for (int i = 0; i < 8; i++) tasks[prev].regs[9 + i] = frame[8 + i];
        
        // Salvos s2 - s11
        for (int i = 0; i < 10; i++) tasks[prev].regs[17 + i] = frame[16 + i];
        
        // Temporários t3 - t6
        for (int i = 0; i < 4; i++) tasks[prev].regs[27 + i] = frame[26 + i];

        // --- TROCA DE TAREFA ---
        tasks[prev].state = TASK_READY;
        current = next;
        tasks[next].state = TASK_RUNNING;

        /* Restaura os dados da NOVA tarefa vindos do regs[] 
           para cima do frame que o trap_entry.S vai descarregar na CPU
        */
        frame[0]  = tasks[next].regs[0];   // ra
        frame[30] = tasks[next].regs[1];   // sp
        frame[1]  = tasks[next].regs[2];   // gp
        frame[2]  = tasks[next].regs[3];   // tp
        
        for (int i = 0; i < 3; i++) frame[3 + i]   = tasks[next].regs[4 + i];
        frame[6]  = tasks[next].regs[7];   // s0
        frame[7]  = tasks[next].regs[8];   // s1
        for (int i = 0; i < 8; i++) frame[8 + i]   = tasks[next].regs[9 + i];
        for (int i = 0; i < 10; i++) frame[16 + i] = tasks[next].regs[17 + i];
        for (int i = 0; i < 4; i++) frame[26 + i]  = tasks[next].regs[27 + i];

        // Força a CPU a retornar para a instrução correta da nova task
        asm volatile("csrw sepc, %0" :: "r"(tasks[next].entry));
    }
}

void scheduler_start()
{
    int first_task = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].entry != 0) {
            first_task = i;
            break;
        }
    }

    if (first_task == -1) return;

    current = first_task;
    tasks[current].state = TASK_RUNNING;

    // Habilita as interrupções antes de passar o controle
    asm volatile("csrs sstatus, %0" :: "r"(1 << 1));

    void (*start_entry)(void) = tasks[current].entry;
    start_entry();

    while(1);
}