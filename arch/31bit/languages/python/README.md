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
`mkcmspy.py` applies it until mikebeer/python has it.

## Tests (tc23, tc24, 12 October 2026)

See the test log comparison in docs (9.15 row of the milestone plan).
Differences that are not differences: the console turns tabs into blanks
and splits lines longer than 130 characters. `15_import` needs its
package directory (`tests/pkg`) as CMS files.
