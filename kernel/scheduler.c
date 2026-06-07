#include "scheduler.h"
#include "task.h"

extern void context_switch(void*, void*);

static int current = 0;

static int round_robin()
{
    int next = (current + 1) % task_count;
    int checked = 0;

    /* Procura a próxima task que tenha uma função válida */
    while (tasks[next].entry == 0 && checked < task_count)
    {
        next = (next + 1) % task_count;
        checked++;
    }

    return next;
}

static sched_algo_t current_algo = round_robin;

void scheduler_set_algorithm(sched_algo_t algo)
{
    if (algo)
        current_algo = algo;
}

void yield()
{
    int prev = current;
    int next = current_algo();

    if (prev != next) 
    {
        current = next;
        context_switch(tasks[prev].regs, tasks[next].regs);
    }
}
 
void scheduler_start()
{
    if (task_count == 0)
        return;

    /* Encontra a primeira task que ainda está viva (entry != 0) */
    for (int i = 0; i < task_count; i++)
    {
        if (tasks[i].entry != 0)
        {
            current = i;
            tasks[i].entry(); /* Inicia a task e nunca volta pra ca */
        }
    }
    /* Se chegar aqui, todas as tasks foram deletadas */
    while(1);
}