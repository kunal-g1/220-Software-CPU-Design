; hello.asm — prints "Hello, World!\n"
.org $8000
start:
    LDA #72     ; 'H'
    OUT
    LDA #101    ; 'e'
    OUT
    LDA #108    ; 'l'
    OUT
    OUT         ; 'l'
    LDA #111    ; 'o'
    OUT
    LDA #44     ; ','
    OUT
    LDA #32     ; ' '
    OUT
    LDA #87     ; 'W'
    OUT
    LDA #111    ; 'o'
    OUT
    LDA #114    ; 'r'
    OUT
    LDA #108    ; 'l'
    OUT
    LDA #100    ; 'd'
    OUT
    LDA #33     ; '!'
    OUT
    LDA #10     ; '\n'
    OUT
    HLT
