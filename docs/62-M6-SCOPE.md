# M6 — 31-bit CMS proper and larger minidisks (scoped 11 October 2026)

M5 made CMS usable above 16 MB without converting it (doc 36): the nucleus
stays below the line, programs that ask run AMODE 31 and get storage above
it. M6 covers the two things M5 left out on purpose.

## A. Larger minidisks and larger files: the file system

VM/370 Release 6 CMS knows one disk format: **800-byte blocks**. There is no
EDF; FORMAT writes 800-byte records only. The format's limits all come from
halfword fields:

| Field | Width | Limit it sets |
|---|---|---|
| `FSTRECCT` (records in a file) | H | 65,535 records. This is why the Linux kernel deck is split into KERNEL1 and KERNEL2. |
| `FSTBLKCT` (blocks in a file), `FSTFCLPT` (first chain link) | H | 65,535 blocks = 52 MB a file |
| Chain links and the allocation map (QMSK) | H block numbers | 65,535 blocks = about 52 MB a disk, about 95 cylinders of a 3350 |
| `FSTWRPNT` / `FSTRDPNT` | H | item pointers |

A larger disk or file needs a second format. IBM's answer was the **EDF**
(Enhanced Disk Format, VM/SP), and we would follow it:

- block sizes 1K, 2K and 4K, set by `FORMAT ... (BLKSIZE n`;
- 64-byte FSTs with fullword counts and pointers;
- the FST directory and the allocation map kept as files;
- the 800-byte format kept alongside it: every existing disk stays readable,
  and ACCESS tells the two formats apart by the label record.

Our CMS code is 31-bit-capable after M5. EDF is a file-system implementation
in the DMS modules: 34 modules use the FST DSECTs and 56 the ADT. The core
modules are DMSACC (access), DMSFOR (format), DMSDIO (disk I/O), DMSRWS and
DMSFNS (read/write), DMSALU and DMSTQQ (allocation), DMSERS (erase) and
DMSAUD (update). CP is untouched: a CMS minidisk is only a range of
cylinders to CP.

Proposed increments:

| | Increment | Observable |
|---|---|---|
| M6a | FORMAT (BLKSIZE 4096) writes an EDF label, an FST directory and an allocation map | `FORMAT 192 B (BLKSIZE 4096` on a 3350 TDISK; LISTFILE shows an empty disk |
| M6b | ACCESS and LISTFILE of an EDF disk, read-only | a disk prepared on the PC side (tools/edf.py) lists and types |
| M6c | write, erase and rename on EDF | COPYFILE of a 100,000-record file; the regression unchanged on 800-byte disks |
| M6d | large minidisks: full-volume 3390 (cylinder counts above 65,535 blocks) | a 1,000-cylinder 3390 minidisk holds a 200 MB file |
| M6e | the tools: DISK DUMP/LOAD, TAPE, MOVEFILE, FULIST across both formats | the kit's tests |

## B. The 31-bit nucleus

The nucleus code still has the 156 `LA` strips and 34 three-byte `ICM` sites
that M5 left alone (they never see a high address while the nucleus, PLISTs
and FSCBs stay below). Converting them is only needed if the nucleus itself
moves above the line or AMODE 31 callers pass high PLISTs. This is the same
sweep CP had (I-126, I-233, I-268). Until a user-visible need appears it
stays a background item: high data is reachable through M5's bridges today.

## Effort and order

A is several days of CMS file-system work, comparable to M4b.3 on CP. B is
one day of sweep plus regression. Suggested order: A (M6a to M6c first; they
also lift the 65,535-record limit), then B if a caller needs it.
