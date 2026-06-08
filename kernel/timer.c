#include "timer.h"

static uint64_t timer_interval = 0;

void timer_init(uint64_t interval)
{
    timer_interval = interval;
    
    // Configura o primeiro evento do timer somando o tempo atual com o intervalo
    timer_next();

    // Habilita a interrupção por Timer em modo supervisor (STIE) no registrador sie
    asm volatile("csrs sie, %0" :: "r"(1 << 5));

    // Habilita interrupções globais (SIE) no registrador sstatus
    asm volatile("csrs sstatus, %0" :: "r"(1 << 1));
}

void timer_next(void)
{
    uint64_t current_time;
    // Lê o contador de tempo físico do hardware
    asm volatile("csrr %0, time" : "=r"(current_time));

    uint64_t next_event = current_time + timer_interval;

    register uint64_t a0 asm("a0") = next_event;
    register uint64_t a7 asm("a7") = 0; 
    asm volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
}