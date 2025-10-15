; Test all legal addressing mode combinations
.entry MODES

; mov - all source modes, no immediate dest
MODES: mov #5, r1
       mov r1, r2
       mov VAR, r3
       mov MAT[r1][r2],r4
       
; lea - only direct/matrix source
       lea VAR, r5
       lea MAT[r0][r1],r6
       
; cmp - all modes for both operands
       cmp #5, #10
       cmp r1, VAR
       cmp MAT[r1][r2], MAT[r3][r4]
       
; prn - all destination modes
       prn #42
       prn r7
       prn VAR
       prn MAT[r2][r3]
       
VAR:   .data 100
MAT:   .mat [3][3]
