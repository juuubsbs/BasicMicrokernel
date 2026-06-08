#include "trap.h"
#include "timer.h"
#include "scheduler.h"
#include "uart.h"

static uint32_t timer_ticks = 0;

void trap_handler(uint64_t *frame)
{
    uint64_t scause;
    asm volatile("csrr %0, scause" : "=r"(scause));

    if ((scause >> 63) & 1)
    {
        uint64_t exception_code = scause & 0xfff;
        
        if (exception_code == 5)
        {
            timer_ticks++;

            // Se bater 20 alternâncias, desliga o QEMU imediatamente
            if (timer_ticks >= 20)
            {
                uart_print("\n[KERNEL] Tempo esgotado! Desligando...\n");
                
                // Endereço físico do dispositivo "Test" na placa 'virt' do QEMU
                volatile uint32_t *qemu_shutdown = (volatile uint32_t *)0x100000;
                
                // O código 0x5555 diz ao QEMU para fechar a janela/encerrar o processo
                *qemu_shutdown = 0x5555; 
                
                while(1); // Linha de segurança caso o emulador demore a fechar
            }

            timer_next();
            schedule_from_trap(frame);
        }
    }
}