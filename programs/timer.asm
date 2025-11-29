; timer.asm — demonstrates fetch/compute/store with --trace
.org $8000
start:
    LDA #5       ; set A=5
    STA $10      ; store to zp10
    LDB #1
loop:
    ADD          ; A = A + B
    STA $10      ; store back
    DEC          ; A = A - 1 (simulate some work)
    JNZ loop
    HLT
