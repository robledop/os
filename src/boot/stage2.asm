[BITS 32]

_start:
    extern load_kernel
    call load_kernel

null_loop:
    hlt
    jmp null_loop

