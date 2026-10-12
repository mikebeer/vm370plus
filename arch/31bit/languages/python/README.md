# Python on CMS (9.15): mikebeer/python

The C89 interpreter of `github.com/mikebeer/python` (a Python 3 subset with
its library -- NumPy, pandas, SciPy, re, json, struct, cmath ... -- compiled
in, `-DPY_EMBED_LIBS`) runs on VM/370plus CMS.

    PYTHON fn [args]     runs fn PY from any accessed disk
    PYTHON               interactive; a line of `/*` ends the input

`mkcmspy.py PYSRC OUT` builds it with the M5f toolchain (`m5f/cmscc`, GCC 13
-m31, newlib, cmsrt) for the XL loader: `PY XIMAGE` (2.3 MB, more than a
TEXT deck holds), started by `PYTHON EXEC` as `XL PY ...` with the
arguments through the stack. It also writes the edf.py putlist for the
EXECs and the tests (`NN_name.py` as `PYTnn PY`), and `map.txt`.
`PYTEST EXEC` runs a list of them.

Needs: `CP DEFINE STORAGE 256M`, `IPL 290`, the XL loader (XL MODULE).

## The 32-bit long

On CMS `long` has 32 bits, so the interpreter's small ints are limited to
+-999,999,999 and larger values are big integers. Converting a float to an
int tested against 1e17 and cast to long, which saturated at 2147483647 for
anything above 2^31 (`int(1e10)`, `Fraction(0.25)`). `python-32bit-long.patch`
tests against `SI_MAX` instead (both word sizes; 29 of 29 tests on the PC);
It is in mikebeer/python since commit 6e4a7a5 (12 Oct); `mkcmspy.py` applies
the patch only to an older checkout.

## Tests (tc23, tc24, 12 October 2026)

27 of the 29 tests give CPython's output on CMS (NumPy, pandas, SciPy,
FFT, re, json, complex, classes, generators, exceptions ...), counting the
console's own changes as equal: it turns tabs into blanks, splits lines
longer than 130 characters, and cannot show non-ASCII text (17_language2
prints an `É` that the 3215 shows as nothing).
- `15_import`: needs its package directory (`tests/pkg`) as CMS files.
- `24_sockets`: no sockets on CMS (9.5).
