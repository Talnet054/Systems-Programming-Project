; This file demonstrates error detection
mov #513, r1    ; Error: immediate value out of range [-512, 511]
add UNDEFINED, r2  ; Error: undefined symbol
jmp              ; Error: missing operand
mov r1, r9       ; Error: invalid register (only r0-r7)
