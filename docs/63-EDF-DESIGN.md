# EDF in VM/370plus CMS (M6, 11 October 2026)

Doc 62 covers the scope and the reasons. This document records how EDF
(IBM's Enhanced Disk Format) was put into Release 6 CMS, as built and
tested.

The on-disk format is IBM's. `tools/edf.py` is the PC-side reference: it
formats, lists, reads and writes EDF disks, so every byte CMS writes can be
checked.

## On disk (IBM's layout)

- **Blocks.** Every track holds fixed 4096-byte records after R0. Block n
  (counting from 1) is record `(n-1) % bpt + 1` of track `(n-1) // bpt`.

  | Device | Blocks per track | Tracks per cylinder |
  |---|---|---|
  | 3350 | 4 | 30 |
  | 3380 | 10 | 15 |

- **Label.** Blocks 1–2 are IPL records; block 3 is the label. It holds
  `'CMS1'`, the volid and the version, then these fullwords:
  - block size
  - DOP (= 4)
  - cylinders formatted, and the maximum
  - total blocks
  - used blocks
  - FST size (64)
  - FSTs per block

  The 6-byte BCD date is at +44.
- **Directory.** The directory is itself a file, whose first block is DOP.
  - Its first entry is its own FST (X'0000000100000000' `DIRECTOR`).
  - The second is the allocation map (X'0000000200000000' `ALLOCMAP`).
  - The user files follow. A slot with zeros in its first 8 bytes is free.
- **64-byte FST.** The first 40 bytes are the 800-byte format's FST:
  - name and type
  - date (MMDDHHMM in BCD)
  - write and read pointers
  - mode
  - halfword item count
  - halfword FOP
  - RECFM
  - flags (X'08': 20xx)
  - LRECL
  - halfword block count
  - year (EBCDIC)

  Then fullwords FOP (+40), ADBC (+44) and AIC (+48), the level count (+52),
  the pointer size (+53, 4 for F, 12 for V) and a 6-byte BCD date (+54).
- **Files are trees.**
  - Level 0: FOP is the data block.
  - Otherwise FOP is a pointer block.
    - F files: 4-byte entries, 1024 to a block.
    - V files: 12-byte entries (block, highest record starting in it,
      displacement of the first record start, X'FFFFFFFF' if none), 341 to
      a block. The last word of a V pointer block holds the offset of its
      last entry.
  - Zero pointers are null blocks.
- **V data.** V records are a 2-byte length followed by the data, packed
  across blocks. A zero length (padding) ends the data.
- **ALLOCMAP.** One bit per block, MSB first, set = in use. Blocks past the
  end of the disk are set too.

## In CMS: one nucleus extension and small hooks

The rule: **everything that only looks at FSTs keeps working unchanged; only
the code that moves data blocks learns EDF.**

### DMSEFS

DMSEFS (`cms/DMSEFS.ASSEMBLE`) is a nucleus extension.

- **Loading it:** `NUCXTEXT DMSEFS (SYSTEM PERM`, then the command `DMSEFS`.
  That puts its service entry in NUCON `NUCRSV7`.
- **Calling it:** the nucleus calls `L R15,NUCRSV7` / `BALR R14,R15`, with
  R0 = function, R1 = argument and R2 = second argument. It returns R15 = RC
  with the condition code set, and R0–R14 unchanged.

| Fn | Name | Called from | Does |
|---|---|---|---|
| 1 | CHK | DMSACM (READMFD), after DIAG X'24' | Reads block 3; on `'CMS1'` sets up the ADT: ADTFLG3 X'02', block counts, and the EDFCB in ADTQQM. Then reads the whole directory, and for R/W disks also the map and its block list. Returns 255 if the disk is not EDF. |
| 2 | FSTS | DMSACF (READFST) | Builds the 40-byte FST hyperblocks from the directory. Counts are capped at 65,535, and FSTFCL holds the directory index. Then SORTFST runs. |
| 3 | READ | DMSBRD at RDB03 | RDBUF for F and V files, through the tree. |
| 4 | CLOSE | DMSFNS FRETEM | Returns an open file's buffers. |
| 5 | WRITE | DMSBWR at PROCES | WRBUF. F files at any item; V files appended. |
| 6 | FINW | DMSFNS WRITEM3 | An output file is finished: its pointer tree, its directory entry, then COMMIT. |
| 7 | SYNC | DMSAUD UPDISK | Entries whose FST is gone (erased files) give their blocks back; renamed files get their new names. Then COMMIT. |
| 8 | FMT | DMSFOR (`FORMAT ... (BLKSIZE 4096`) | Probes the cylinders, formats the tracks, and writes the label, directory and map. |
| 9 | REL | DMSALU RELUFD | Returns everything EDF holds for the disk. |

### What DMSEFS keeps

- **Per disk (EDFCB):**
  - the directory in storage (64 bytes an entry)
  - the allocation map and its block numbers
  - a read cache for pointer blocks
  - a 4K buffer for writing
  - an allocation cursor
- **Per open file:** AFTCLB (80 bytes) holds the EDF state:
  - FOP, ADBC, AIC, levels and pointer size
  - read and write pointers as fullwords
  - the data-block buffer and the pointer-block buffer
  - for output, a block list of 12-byte entries (block, highest record,
    displacement)

### Writing

- New blocks are allocated from the in-storage map as data reaches them.
- An existing file's tree is read into the block list when it is first
  written, and its pointer blocks are freed.
- The new tree is built at FINIS.
- COMMIT writes, in this order:
  1. the directory, with its own tree (DOP stays its first block)
  2. the map
  3. the label's used count

### Item numbers

CMS PLISTs carry halfword item numbers. DMSEFS turns them into fullword item
numbers by three rules:

- 0 means the pointer, as before.
- The caller's last item + 1 also means the pointer. A sequential caller
  counting in halfwords therefore just continues. This holds even when its
  count is not the file's: `COPYFILE (APPEND` starts from the FST's count,
  which is capped at 65,535.
- Any other number is taken modulo 65,536, nearest the pointer. WRBUF's own 65,535 check moved to PROCES, where the disk
is known; 800-byte disks keep it.

### I/O

All I/O is DIAG X'20' on the virtual device: SEEK, SEARCH ID EQUAL, TIC, then
READ or WRITE DATA of 4096 bytes. DIAG X'18' takes at most 2048 bytes per
CCW, and DMSDIO is untouched.

FORMAT searches for R0 and writes the records with Write CKD, as DMSFOR does.
CP refuses Home Address and R0 writes on minidisks.

## The transient-area constraint

DMSACC, DMSACF, DMSACM and DMSALU are loaded together into CMS's 8K transient
area (E000–FFFF). DMSALU's `END$RELU` says it must not pass X'10000'.

The hooks first pushed the modules past that boundary into low free storage,
and IPL failed with DMSFRE161T. DMSALU's 1600-byte SORTFST merge buffer
therefore now comes from DMSFREE for the duration of a merge. That made room.

The ACCESS, RELEASE and FORMAT modules are generated from the TEXT decks by
`CMSGEND ACCESS|RELEASE|FORMAT`. They must be regenerated whenever DMSACC,
DMSACF, DMSACM, DMSALU, DMSARE or DMSFOR change.

## Status (11 October)

| | Done | Evidence |
|---|---|---|
| M6b read | ACCESS, LISTFILE, STATE, TYPE, COPYFILE from an EDF disk | 100,000-record V and 70,000-record F files (written by edf.py) |
| M6c write | COPYFILE to the disk, RENAME, ERASE, re-ACCESS | a 70,000-record copy is byte-identical (edf.py get); erased files' blocks are all free again |
| M6a FORMAT | `FORMAT vaddr fm (BLKSIZE 4096` on a T3350 temporary disk and on a dedicated 3380 | 64 files across two directory blocks survive RELEASE/ACCESS |
| M6d large | a full 885-cylinder 3380 (132,750 blocks, 518 MB) formatted by CMS; one F file of 1,400,000 records (112 MB, 27,344 blocks, two pointer levels) built by COPYFILE (APPEND | edf.py get: byte-identical to 20 copies of the source |

Open:
- LISTFILE, STATE and TYPE show counts capped at 65,535, and TYPE cannot
  address lines past 65,535.
- QUERY DISK shows Blksize 800.
- DMSEFS still has to be loaded by hand (it should come from SYSPROFX).
- The kit build needs the CMSGEND step.
- V files can only be appended to.
- Files are written mode `P` in the directory, as FINIS does for 800-byte
  disks.
