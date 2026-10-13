; HELLO.ASM -- an 8080 program for CP/M 2.2 on VM/370plus (9.33)
; CPM PUT HELLO ASM, then CPM ASM HELLO; LOAD HELLO; HELLO
BDOS    EQU     5
PRINT   EQU     9
        ORG     100H
        MVI     C,PRINT
        LXI     D,MSG
        CALL    BDOS
        RET
MSG:    DB      'Hello from CP/M 2.2 on VM/370plus!',13,10,'$'
        END
