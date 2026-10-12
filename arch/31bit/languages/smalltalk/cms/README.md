# Smalltalk on CMS and Debian (VM/370plus)

Upstream: github.com/mikebeer/smalltalk (see `../UPSTREAM`). There are two
interpreters, and both read the same class library (`../lib/*.st`):

| | CMS | Debian |
|---|---|---|
| C89 (`../c/st_all.c`) | SMALLTC MODULE (`../mkstc.sh` -> stc.text; LOAD, GENMOD) | `stc` (`../mkstc.sh`, wcc) |
| cREXX | SMALLTLK RXBIN on RXBVM8 | `smalltalk` (rxvm) |

`SMALLTLK fn.st` runs SMALLTC when it is on an accessed disk, and otherwise
the cREXX version. `SMALLTLK -i` gives the interactive prompt. The library
files are `KERNEL ST`, `NUMBERS ST`, `COLLECTI ST`, `STREAMS ST`, `EXCEPTIO ST`,
`EXTRAS ST`, `STRUCTUR ST`, `REGEX ST`, `SYSTEM ST`, `NET ST`, `SUNIT ST`,
`PROCESS ST`, `MORE ST` and `MORE2 ST` (V format; names are cut to 8).

`STTEST ST01 ST02 ...` runs the shared tests (`tests/NN_name.st` as `STNN ST`,
with `STNN IN` and `STNN FLAGS`), and writes each one's output to the console
between `=== STTEST` and `=== END` lines.

## Results (12 October 2026, upstream ab091fc)

| | passed | differences |
|---|---|---|
| CMS (tc42/43) | 20 of 22 | `06_numbers`: SmallIntegers are +-2^30 with a 32-bit long (documented upstream); `14_files_system`: no directories on CMS. `05_strings` also differs, but only because the console shows a TAB as a blank. |
| Debian (pd2) | 21 of 22 | `06_numbers`, as above |

`16_net` (sockets) and `23_unicode` (the cREXX VM's Unicode) are not part
of the C version. Files under `/tmp/` are CMS files on the A disk, named by
the last component (cmsrt).
