; Test complex scenarios from PDF specs
.entry COMPLEX
.extern PRINTNUM
.extern READNUM

COMPLEX: lea ARRAY, r1
         mov ARRAY[r2][r3], r4
         add #-5, r4
         cmp r4, #10
         bne SKIP
         prn ARRAY[r1][r2]
         
SKIP:    not r5
         clr ARRAY[r0][r1]
         inc r6
         dec COUNT
         red r7
         jsr PRINTNUM
         stop
         
ARRAY:   .mat [4][4] 1,2,3,4,5,6,7,8
COUNT:   .data 10
