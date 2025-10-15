; Test boundary values
.entry BOUNDS
BOUNDS: mov #511, r0
 mov #-512, r1
 mov r0, r7
 add r7, r0
DATA: .data 511, -512, 0, 1, -1
STR: .string "Hello, World! 123"
BIG: .mat [10][10]
