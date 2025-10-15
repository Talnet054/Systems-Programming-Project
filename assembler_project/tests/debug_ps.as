.extern W
MAIN: mov M1[r2][r7],W
SECOND: add r2,STR
STR: .string "test"
M1: .mat [2][2] 1,2,3,4
