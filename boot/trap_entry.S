.section .text
.global trap_entry
.align 4

trap_entry:
    # 1. Reserva espaço na stack para 31 registradores de 64 bits (31 * 8 = 248 bytes)
    # Alinhado para 256 bytes para respeitar a ABI (16 bytes align)
    addi sp, sp, -256

    # 2. Salva os registradores x1 a x31 (pulando x0 que é sempre zero)
    sd x1,   0(sp)
    sd x3,   8(sp)   # gp
    sd x4,   16(sp)  # tp
    sd x5,   24(sp)  # t0
    sd x6,   32(sp)  # t1
    sd x7,   40(sp)  # t2
    sd x8,   48(sp)  # s0/fp
    sd x9,   56(sp)  # s1
    sd x10,  64(sp)  # a0
    sd x11,  72(sp)  # a1
    sd x12,  80(sp)  # a2
    sd x13,  88(sp)  # a3
    sd x14,  96(sp)  # a4
    sd x15, 104(sp)  # a5
    sd x16, 112(sp)  # a6
    sd x17, 120(sp)  # a7
    sd x18, 128(sp)  # s2
    sd x19, 136(sp)  # s3
    sd x20, 144(sp)  # s4
    sd x21, 152(sp)  # s5
    sd x22, 160(sp)  # s6
    sd x23, 168(sp)  # s7
    sd x24, 176(sp)  # s8
    sd x25, 184(sp)  # s9
    sd x26, 192(sp)  # s10
    sd x27, 200(sp)  # s11
    sd x28, 208(sp)  # t3
    sd x29, 216(sp)  # t4
    sd x30, 224(sp)  # t5
    sd x31, 232(sp)  # t6

    # O registrador original SP (x2) antes do trap precisa ser salvo manualmente.
    # Como decrementamos 256, o SP antigo é (sp atual + 256).
    addi t0, sp, 256
    sd t0, 240(sp)   # Salva o SP antigo na posição reservada

    # 3. Passa o ponteiro da base do frame no registrador a0 como argumento para a função C
    mv a0, sp

    # 4. Chama o manipulador central de interrupções em C
    call trap_handler

    # 5. Restaura todos os registradores (que podem ter sido alterados pelo scheduler)
    ld x1,   0(sp)
    ld x3,   8(sp)
    ld x4,   16(sp)
    ld x5,   24(sp)
    ld x6,   32(sp)
    ld x7,   40(sp)
    ld x8,   48(sp)
    ld x9,   56(sp)
    ld x10,  64(sp)
    ld x11,  72(sp)
    ld x12,  80(sp)
    ld x13,  88(sp)
    ld x14,  96(sp)
    ld x15, 104(sp)
    ld x16, 112(sp)
    ld x17, 120(sp)
    ld x18, 128(sp)
    ld x19, 136(sp)
    ld x20, 144(sp)
    ld x21, 152(sp)
    ld x22, 160(sp)
    ld x23, 168(sp)
    ld x24, 176(sp)
    ld x25, 184(sp)
    ld x26, 192(sp)
    ld x27, 200(sp)
    ld x28, 208(sp)
    ld x29, 216(sp)
    ld x30, 224(sp)
    ld x31, 232(sp)

    # 6. Libera o espaço de contexto da pilha
    addi sp, sp, 256

    # 7. Executa o retorno do modo supervisor, saltando para o endereço contido em sepc
    sret