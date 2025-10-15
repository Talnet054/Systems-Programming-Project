; Test edge cases from PDF
.entry EDGE
.extern EXTERNAL

; Test all registers
EDGE: mov r0, r1
      mov r7, r6
      
; Test matrix edge cases
MAT1: .mat [1][1] 511
MAT2: .mat [2][2] -512
MAT3: .mat [5][5]

; Test string edge cases  
STR1: .string ""
STR2: .string "a"
STR3: .string "Test 123!@#"

; Test immediate boundaries
      mov #511, r0
      mov #-512, r1
      mov #0, r2
      
; Test all jump instructions
      jmp EXTERNAL
      bne EDGE
      jsr EDGE
      
; Test data with patterns
DATA: .data 1, -1, 2, -2, 4, -4, 8, -8
