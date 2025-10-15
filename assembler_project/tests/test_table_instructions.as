; Test instructions from PDF table
.entry MAIN
MAIN: clr r2       ; opcode 5
      not r2       ; opcode 4  
      inc r2       ; opcode 7
      dec Count    ; opcode 8
      jmp Line     ; opcode 9
Line: bne Line     ; opcode 10
      jsr SUBR     ; opcode 13
      red r1       ; opcode 11
      prn r1       ; opcode 12
      stop
SUBR: rts
Count: .data 5
