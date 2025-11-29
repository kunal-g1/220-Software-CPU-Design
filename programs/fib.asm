; fib.asm — print first 10 Fibonacci numbers as raw bytes (0..255), separated by spaces, newline at end
; On most terminals these will not be human-friendly; but they satisfy the emulator IO write.
; To make it human-friendly, run with a hex-view filter, or extend ISA.
.org $8000
start:
    LDA #0       ; a
    LDB #1       ; b
    STA $00      ; zp a
    STA $02      ; space char later
    LDA #10
    STA $01      ; count = 10
print_loop:
    LDA $00      ; print 'a'
    OUT
    LDA #32      ; space
    OUT
    ; next = a + b
    LDA $00
    LDB $03      ; b in zp3 (init below)
    ADD          ; A = a+b
    STA $04      ; next
    ; a = b
    LDA $03
    STA $00
    ; b = next
    LDA $04
    STA $03
    ; dec count and loop
    LDA $01
    DEC
    STA $01
    JNZ print_loop
    LDA #10
    OUT
    HLT
init:
    ; store initial b in zp3 then jump to start
    LDA #1
    STA $03
    JMP start
