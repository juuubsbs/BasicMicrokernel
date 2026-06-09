#include "task.h"
#include "scheduler.h"
#include "memory.h"
#include "uart.h"
#include "timer.h"

extern void trap_entry(void);

/* Tasks para teste */
void task1() 
{
    while(1) {
        uart_print("[TASK 1] Processando via preempcao de hardware...\n");
        for (volatile int i = 0; i < 5000000; i++); 
    }
}

void task2()
{
    while (1)
    {
        uart_print("[TASK 2] Processando via preempcao de hardware...\n");
        for (volatile int i = 0; i < 5000000; i++); 
    }
}

void task_test() {
    while(1) {
        for (volatile int i = 0; i < 100000; i++);
    }
}

void kernel_main()
{
    asm volatile("csrw stvec, %0" :: "r"(trap_entry));
    memory_init();
    timer_init(2000000);

    uart_print("\n=== VALIDACAO DO GERENCIADOR DE MEMORIA ===\n");

    // 1. Estatísticas Iniciais
    uart_print("Heap total: "); uart_print_uint(memory_total()); uart_print(" bytes\n");
    uart_print("Heap usado: "); uart_print_uint(memory_used());  uart_print(" bytes\n");
    uart_print("Heap livre: "); uart_print_uint(memory_free());  uart_print(" bytes\n");
    uart_print("Fragmentacao: "); uart_print_uint(memory_fragmentation()); uart_print("%\n");
    memory_dump();

    // 2. Múltiplas Alocações e Divisão de Blocos (Split)
    xTaskCreate(task_test, 1024, 1); 
    uart_print("Task 0 criada (1024)\n");
    
    xTaskCreate(task_test, 2048, 1); 
    uart_print("Task 1 criada (2048)\n");
    
    xTaskCreate(task_test, 1024, 1); 
    uart_print("Task 2 criada (1024)\n");

    uart_print("Heap total: "); uart_print_uint(memory_total()); uart_print(" bytes\n");
    uart_print("Heap usado: "); uart_print_uint(memory_used()); uart_print(" bytes\n");
    uart_print("Heap livre: "); uart_print_uint(memory_free()); uart_print(" bytes\n");
    uart_print("Fragmentacao: "); uart_print_uint(memory_fragmentation()); uart_print("%\n");
    memory_dump();

    // 3. Liberação de Memória
    xTaskDelete(1); 
    uart_print("Task 1 removida\n");

    uart_print("Heap total: "); uart_print_uint(memory_total()); uart_print(" bytes\n");
    uart_print("Heap usado: "); uart_print_uint(memory_used()); uart_print(" bytes\n");
    uart_print("Heap livre: "); uart_print_uint(memory_free()); uart_print(" bytes\n");
    uart_print("Fragmentacao: "); uart_print_uint(memory_fragmentation()); uart_print("%\n");
    memory_dump();

    // 4. Reutilização de Blocos (First-Fit)
    xTaskCreate(task_test, 512, 1); 
    uart_print("Task 3 criada (512)\n");
    
    uart_print("Heap total: "); uart_print_uint(memory_total()); uart_print(" bytes\n");
    uart_print("Heap usado: "); uart_print_uint(memory_used()); uart_print(" bytes\n");
    uart_print("Heap livre: "); uart_print_uint(memory_free()); uart_print(" bytes\n");
    uart_print("Fragmentacao: "); uart_print_uint(memory_fragmentation()); uart_print("%\n");
    memory_dump();

    // 5. Coalescência
    xTaskDelete(0);
    uart_print("Task 0 removida\n");

    xTaskDelete(1);
    uart_print("Task 1 removida\n");
    
    xTaskDelete(2);
    uart_print("Task 2 removida\n");
    
    xTaskDelete(3); 
    uart_print("Task 3 removida\n");

    uart_print("Heap total: "); uart_print_uint(memory_total()); uart_print(" bytes\n");
    uart_print("Heap usado: "); uart_print_uint(memory_used()); uart_print(" bytes\n");
    uart_print("Heap livre: "); uart_print_uint(memory_free()); uart_print(" bytes\n");
    uart_print("Fragmentacao: "); uart_print_uint(memory_fragmentation()); uart_print("%\n");
    memory_dump();

    uart_print("===========================================\n\n");

    // Inicialização definitiva do ambiente de execução preemptivo
    uart_print("Iniciando o Escalonador Preemptivo Round-Robin...\n");
    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    scheduler_start();
}