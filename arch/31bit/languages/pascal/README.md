# New Stanford Pascal on CMS (9.13a)

    PCPAS fn         compile fn PASCAL (2026.10 compiler) -> fn PCODE, fn LISTING
    PCRUN fn [args]  run fn PCODE

The New Stanford Pascal compiler of Bernd Oppolzer
(github.com/StanfordPascal/Pascal, release 2026.10) runs on VM/370plus CMS
through PCINT, the release's own P-code interpreter, cross-built for CMS
with the M5f toolchain (`mkpascal-cms.sh`): 31-bit, its storage above
16 MB. The compiler (PASCAL1 with PASSCAN) and the run-time library
(PASLIBX, PASUTILS) are the release's P-code. CE's Stanford Pascal 2021.03
(PASCAL EXEC: P-code translated to 370 code, AMODE 24) is unchanged.

Test tc33 (12 October 2026): `PCPAS PASTEST` (records, sets of char,
strings, recursion, real arithmetic) and `PCRUN PASTEST` give the PC's
output (`tests/pastest.pc`).

## PCINT on a big-endian machine

PCINT keeps a char or boolean on its evaluation stack as an int, and wrote
and read the char at the slot's first byte -- the low-order byte only on a
little-endian PC. On S/390 every char that went through the stack became
0 or c*2^24 (the compiler's first READ of an integer failed, error 1203).
`pcint-bigendian.patch` puts it at the slot's low-order byte (`CHOFF`, from
`__BYTE_ORDER__`) and takes the exponent of a double (XPO) from its bits.
Checked under qemu-s390x: PCINT compiles PASCAL2, PASFORM and the test to
P-code identical to the PC's, byte for byte.

## Native 370 code for the 2026.10 compiler: not yet

The release's P-code-to-370 translator (`src/pascal2.pas`) is version
2024.02; the 2026.10 compiler writes P-code it cannot read (set constants
carry an origin since 2024.06: `LCA S32,0,C'...'`), and comment lines
(`;`) it takes for instructions (error 606). Small programs without set
constants translate (hello: 332 bytes, no errors). Native code for
current releases needs a current PASCAL2 from upstream.
