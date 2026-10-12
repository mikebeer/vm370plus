# VM/370plus XL: the assembler pieces of a large (high-storage) program.
# No CMS entry and no cms202 here: the XL loader (xl.c), a normal module
# below 16 MB, calls cms_xl_main() in AMODE 31 and provides cms202.
# cms_onstack, setjmp and longjmp as in entry31.s.
 .text
 .balign 8
 .globl cms_onstack
cms_onstack:
 stm %r6,%r15,24(%r15)
 lr %r10,%r15
 lr %r15,%r2
 ahi %r15,-96
 xc 0(4,%r15),0(%r15)
 lr %r1,%r3
 lr %r2,%r4
 lr %r3,%r5
 basr %r14,%r1
 lr %r15,%r10
 lm %r6,%r15,24(%r15)
 br %r14

 .globl setjmp
setjmp:
 stm %r6,%r15,0(%r2)
 lhi %r2,0
 br %r14
 .globl longjmp
longjmp:
 lr %r1,%r2
 lr %r2,%r3
 ltr %r2,%r2
 jnz .Llj1
 lhi %r2,1
.Llj1:
 lm %r6,%r15,0(%r1)
 br %r14
 .section .note.GNU-stack,"",@progbits
