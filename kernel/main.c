#include "task.h"
#include "scheduler.h"
#include "memory.h"
#include "uart.h"
#include "timer.h"
#include "string.h"
#include "treefs.h"

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

    uart_print("\n=== VALIDACAO DO TREEFS ===\n");

    fs_init();

    uart_print("\n[Cenario 1] Listagem da raiz -> ls(\"/\")\n");
    ls("/");

    uart_print("\n[Cenario 2] Criacao de diretorio -> mkdir(\"/home/aluno\")\n");
    mkdir("/home/aluno");
    ls("/home");

    uart_print("\n[Cenario 3] Criacao de arquivo -> create(\"/home/aluno/notas.txt\")\n");
    int fd = create("/home/aluno/notas.txt");
    uart_print("fd (inode) retornado: "); uart_print_uint((uint64_t)fd); uart_print("\n");

    uart_print("\n[Cenario 4] Escrita -> write(fd, \"Sistemas Operacionais\", 22)\n");
    write(fd, "Sistemas Operacionais", 22);
    uart_print("Escrita concluida.\n");

    uart_print("\n[Cenario 5] Leitura -> read(fd, buffer, 22)\n");
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    int lidos = read(fd, buffer, 22);
    buffer[lidos] = '\0';
    uart_print("Conteudo lido: "); uart_print(buffer); uart_print("\n");

    uart_print("\n[Cenario 6] Remocao -> unlink(\"/home/aluno/notas.txt\")\n");
    unlink("/home/aluno/notas.txt");
    uart_print("ls(\"/home/aluno\") apos remocao:\n");
    ls("/home/aluno");

    uart_print("\n[Cenario 7] Navegacao hierarquica -> ls(\"/home\")\n");
    ls("/home");

    uart_print("\n[Cenario 8] Reutilizacao de inodes/blocos liberados\n");
    int fd2 = create("/home/aluno/outro.txt");
    uart_print("Novo fd apos remocao anterior (deve reaproveitar o inode liberado): ");
    uart_print_uint((uint64_t)fd2); uart_print("\n");
    write(fd2, "Reuso de bloco", 14);
    inode_t *outro = path_lookup("/home/aluno/outro.txt");
    uart_print("Bloco de dados reaproveitado: ");
    uart_print_uint((uint64_t)outro->blocks[0]); uart_print("\n");

    uart_print("===========================================\n\n");

    uart_print("\n=== FUNCIONALIDADES EXTRAS (BONUS) ===\n");

    uart_print("\n[Bonus 1] Timestamps (created_at / modified_at)\n");
    int fd_ts = create("/tmp/relatorio.txt");
    uart_print("Logo apos create():\n");
    fs_stat("/tmp/relatorio.txt");
    write(fd_ts, "v1", 2);
    uart_print("Apos write() (modified_at deve mudar):\n");
    fs_stat("/tmp/relatorio.txt");

    uart_print("\n[Bonus 3] Blocos indiretos (arquivo maior que 8 blocos diretos)\n");
    static uint8_t big_out[5000];
    static uint8_t big_in[5000];
    for (uint32_t i = 0; i < sizeof(big_out); i++) big_out[i] = (uint8_t)('A' + (i % 26));

    int fd_big = create("/tmp/arquivo_grande.bin");
    write(fd_big, big_out, sizeof(big_out));

    memset(big_in, 0, sizeof(big_in));
    read(fd_big, big_in, sizeof(big_in));

    int ok = 1;
    for (uint32_t i = 0; i < sizeof(big_out); i++) {
        if (big_out[i] != big_in[i]) { ok = 0; break; }
    }
    uart_print("Arquivo de "); uart_print_uint(sizeof(big_out));
    uart_print(" bytes (8 blocos diretos + indiretos): ");
    uart_print(ok ? "OK, conteudo integro\n" : "FALHA na integridade\n");
    unlink("/tmp/arquivo_grande.bin");

    uart_print("=================================\n\n");

    // Inicialização definitiva do ambiente de execução preemptivo
    uart_print("Iniciando o Escalonador Preemptivo Round-Robin...\n");
    xTaskCreate(task1, 2048, 1);
    xTaskCreate(task2, 2048, 1);

    scheduler_start();
}