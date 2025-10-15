; Test all 16 instructions with various addressing modes
.entry START
.extern EXTFUNC
START: mov #10, r1
 mov r1, r2
 mov VAR, r3
 mov ARR[r1][r2], r4
 cmp #5, VAR
 cmp r1, r2
 add #1, r1
 sub VAR, r2
 not r3
 clr VAR
 lea ARR, r5
 lea ARR[r1][r2], r6
 inc r7
 dec VAR
 jmp LOOP
 bne EXTFUNC
 red r1
 prn #-1
 prn VAR
 prn r2
 jsr SUBRTN
LOOP: stop
SUBRTN: rts
VAR: .data 100, -100, 0
ARR: .mat [3][3] 1,2,3,4,5,6,7,8,9
