# ncfintr.s -- VM/370plus 9.5: NICOF's low-level interface for GCC 13
# (-m31, cmsrt).  Replaces CE's INTRAPI ASSEMBLE for programs built with
# arch/31bit/m5f/crt.sh.
#
# The program runs in AMODE 31 under the EC-mode CMS (IPL 290).  CP's
# DIAGNOSE X'68' (VMCF), DIAGNOSE X'08' and CMS's WAIT and STIMER take
# 24-bit addresses, so ncf_low switches to AMODE 24 for them, like cms202
# in entry31.s; everything they point to must be below 16 MB (static data).
#
# External interrupts reach ncf_exth through DMSITE (HNDEXT SET): in BC
# mode (DMSITE's EXTPSW), R1 -> DMSITE's user area (+96 the external old
# PSW), R13 -> a save area, R14 the return.  It runs no C: it copies the
# VMCF header into a queue and posts the program's ECB.  The interrupt code
# is in the old PSW (bytes 2-3) when that PSW is BC, at X'86' when it is EC
# (I-285).
#
# Released to the public domain, like NICOF.

        .equ NSLOT, 32              # queued VMCF headers (a power of 2)
        .equ HSZ, 176               # bytes per header (VMCMHDR + SMSG text)

# int ncf_low(int op, unsigned a, unsigned b)
#   op 1: DIAG X'68', a = parameter list            -> the VMCF rc
#   op 2: DIAG X'08', a = command, b = length       -> 0
#   op 3: SVC 1 (WAIT), R0 = a, R1 = b              -> 0
#   op 4: SVC 47 (STIMER), R0 = a, R1 = b           -> 0
        .text
        .balign 8
        .globl ncf_low
ncf_low:
        stm   %r6,%r15,24(%r15)
        lr    %r10,%r15
        basr  %r12,0
.Ll:
        l     %r13,.Llsa-.Ll(%r12)  # a CMS save area below 16 MB
        lr    %r7,%r2
        lr    %r8,%r3
        lr    %r9,%r4
        l     %r11,.Ll24-.Ll(%r12)
        bsm   0,%r11                # AMODE 24
.Ll_in24:
        chi   %r7,1
        jne   1f
        lr    %r3,%r8
        l     %r2,.Lones-.Ll(%r12)
        .long 0x83320068            # DIAG 68: R3 = parameters, R2 = rc
        j     9f
1:      chi   %r7,2
        jne   2f
        lr    %r2,%r8
        lr    %r3,%r9
        .long 0x83230008            # DIAG 08: R2 = command, R3 = length
        sr    %r2,%r2
        j     9f
2:      lr    %r0,%r8
        lr    %r1,%r9
        chi   %r7,3
        jne   3f
        svc   1                     # WAIT
        sr    %r2,%r2
        j     9f
3:      svc   47                    # STIMER
        sr    %r2,%r2
9:      l     %r11,.Ll31-.Ll(%r12)
        o     %r11,.Llhi-.Ll(%r12)
        bsm   0,%r11                # AMODE 31
.Ll_in31:
        lm    %r6,%r15,24(%r10)
        br    %r14
        .balign 4
.Llsa:  .long ncf_sa
.Ll24:  .long .Ll_in24
.Ll31:  .long .Ll_in31
.Llhi:  .long 0x80000000
.Lones: .long 0x11111111

# void ncf_cr0(int on): CR0 bit 31, the VMCF external interrupt mask
        .text
        .balign 8
        .globl ncf_cr0
ncf_cr0:
        larl  %r1,ncf_cr0w
        stctl %c0,%c0,0(%r1)
        ltr   %r2,%r2
        jz    1f
        oi    3(%r1),0x01
        j     2f
1:      ni    3(%r1),0xfe
2:      lctl  %c0,%c0,0(%r1)
        br    %r14

# the external interrupt handler (HNDEXT SET): see above
        .text
        .balign 8
        .globl ncf_exth
ncf_exth:
        stm   %r14,%r12,12(%r13)
        tm    97(%r1),0x08          # old PSW: EC mode?
        jz    1f
        la    %r3,0x86
        lh    %r2,0(%r3)            # EC: the code is at X'86'
        j     2f
1:      lh    %r2,98(%r1)           # BC: in the old PSW
2:      chi   %r2,0x4001            # VMCF?
        jne   9f
        larl  %r3,ncf_q
        l     %r4,0(%r3)            # in
        lr    %r5,%r4
        s     %r5,4(%r3)            # - out
        chi   %r5,NSLOT
        jnl   8f                    # full: lost (the C side counts)
        lr    %r5,%r4
        lhi   %r6,NSLOT-1
        nr    %r5,%r6
        mhi   %r5,HSZ
        la    %r5,16(%r5,%r3)       # the slot
        larl  %r6,ncf_hdrp
        l     %r6,0(%r6)
        mvc   0(HSZ,%r5),0(%r6)     # copy the header (and SMSG text)
        ahi   %r4,1
        st    %r4,0(%r3)            # in + 1
        j     7f
8:      l     %r4,8(%r3)
        ahi   %r4,1
        st    %r4,8(%r3)            # lost + 1
7:      larl  %r3,ncf_ecbp
        l     %r3,0(%r3)
        ltr   %r3,%r3
        jz    9f
        xc    0(4,%r3),0(%r3)
        oi    0(%r3),0x40           # POST
9:      lm    %r14,%r12,12(%r13)
        br    %r14

# the STIMER exit: DMSITE calls it (BALR) when the interval ends
        .text
        .balign 8
        .globl ncf_tmrx
ncf_tmrx:
        larl  %r1,ncf_tmrecb
        l     %r1,0(%r1)
        ltr   %r1,%r1
        jz    1f
        xc    0(4,%r1),0(%r1)
        oi    0(%r1),0x40           # POST
1:      br    %r14

        .data
        .balign 8
        .globl ncf_q, ncf_hdrp, ncf_ecbp, ncf_tmrecb
ncf_q:  .long 0, 0, 0, 0            # in, out, lost, -
        .skip NSLOT*HSZ
ncf_hdrp:   .long 0                 # the VMCF header area (AUTHORIZE)
ncf_ecbp:   .long 0                 # the ECB to post on VMCF
ncf_tmrecb: .long 0                 # the ECB the timer posts
ncf_cr0w:   .long 0
        .balign 8
ncf_sa: .skip 96
        .section .note.GNU-stack,"",@progbits
