; Test A,R,E encoding - data should allow all values including those ending in d
.entry MAIN
.extern PRINT
MAIN: mov #-5, r1
 add r1, LABEL
 jmp PRINT
LABEL: .data 3, 7, 15, 31, 63, 127, 255, 511, -1, -256, -511, -512
VALUES: .data 3, 7, 11, 15, 19, 23, 27, 31
STRING: .string "Test"
MATRIX: .mat [2][2] 3, 7, 11, 15
