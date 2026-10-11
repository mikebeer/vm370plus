# EDF in VM/370plus CMS — design (M6, 11 October 2026)

Scope and motivation are in doc 62. This document records how EDF is put into
Release 6 CMS. The on-disk format is IBM's; `tools/edf.py` is the PC-side
reference that formats, lists, reads and writes it, so every byte CMS writes
can be checked.

## On disk (IBM's layout)

- Every track is formatted with fixed 4096-byte records. Block n (1-origin)
  is record `(n-1) % bpt + 1` of track `(n-1) // bpt`. Blocks per track:
  3350 → 4, 3375 → 8, 3380 → 10 (the counts CP's own paging uses).
- Blocks 1–2 are IPL records. Block 3 is the label: `'CMS1'`, volid,
  version, then fullwords: blksize, DOP (=4), cylinders formatted and
  maximum, total blocks, used blocks, FST size (64), FSTs per block, date.
- The directory is a file. Its first entry is its own FST
  (X'0000000100000000' `DIRECTOR`), the second is the allocation map
  (X'0000000200000000' `ALLOCMAP`), then the user files. DOP names its
  first data block.
- The 64-byte FST keeps the 800-byte format's first 40 bytes and adds
  fullwords: FOP (+40), ADBC data blocks (+44), AIC records (+48), then
  levels (+52), pointer size (+53) and a 6-byte BCD date (+54).
- Files are trees.
  - Level 0: FOP is the data block.
  - Otherwise FOP is a pointer block: 4-byte entries for F files; 12-byte
    entries (block, highest record number, displacement) for V files. The
    last word of a V pointer block holds the offset of its last entry.
- V records are a 2-byte length plus data, packed across blocks.
- ALLOCMAP has one bit per block, MSB first, set = in use.

## In CMS: a shim, not a second file system

The rule: **everything that only looks at FSTs keeps working unchanged; only
the code that moves data blocks learns EDF.**

1. **ACCESS (DMSACM, DMSACF).**
   - DMSACM calls `EDFCHK` (new module DMSEDF) before reading the 800-byte
     label. It reads track 0 record 3 with SILI, by DIAG X'20'.
   - On `'CMS1'` the disk is EDF: `ADTFLG3 X'02'` (`ADTFEDF`) is set and
     `EDFACC` fills the ADT.
     - `ADTCYL`, `ADTNUM`/`ADTUSED`/`ADTLEFT` are counted in 4K blocks.
     - `ADTMSK` points to the allocation map in storage (R/W disks).
     - `ADTQQM`, unused on 3350/3375/3380, points to the EDF control
       block `EDFCB` (block size, blocks per track, heads, DOP, the
       directory's own FST, its block list).
   - DMSACF's hyperblock loop gets its 40-byte entries from `EDFNEXT`, which
     reads the directory file and converts each 64-byte FST:
     - the first 40 bytes are copied, with the halfword counts set to
       min(value, 65535);
     - the fullwords go into an *extension* of 24 bytes per slot.
2. **Hyperblocks with extensions.**
   - An EDF hyperblock is 800 bytes of FSTs plus forward and back
     pointers, followed by 20 × 24 bytes of extension (FOP, ADBC, AIC,
     levels, pointer size, date).
   - `EDFXFST` maps an FST address to its extension by walking the chain
     (an address-range test).
   - Allocation sizes change in three places: DMSACF, DMSLFS FINDZERO and
     DMSALU RELUFD.
   - EDF directories are not sorted (DMSALU SORTFST skips them), so FST
     moves happen only in ERASE, which DMSEDF does itself.
3. **Per open file.**
   - AFTCLB (80 bytes, the first chain link's directory, meaningless on
     EDF) becomes the EDF state of an open file:
     - fullword item and write pointers;
     - FOP, ADBC, AIC, levels, pointer size;
     - the current data block and its index;
     - the level-1 pointer block, cached with its index.
   - Buffers are 4K, from DMSFREE.
4. **Read (DMSBRD), write (DMSBWR), FINIS (DMSFNS), ERASE (DMSERS).**
   - Each branches at entry on `ADTFEDF` (via AFTADT) to `EDFRD`, `EDFWR`,
     `EDFFIN` and `EDFERS`. These return exactly what the 800-byte code
     returns.
   - Item numbers are fullwords inside, so more than 65,535 records work
     when reading and writing sequentially.
5. **Directory update (DMSAUD UPDISK)** branches to `EDFUPD`. It rewrites
   the directory blocks from the hyperblocks and extensions, then the
   allocation map, then the label's used count.
6. **Allocation (DMSTRK)** branches to `EDFALC`/`EDFFRE` on the bitmap.
7. **FORMAT (DMSFOR) `(BLKSIZE 4096`** branches to `EDFFMT`. It formats
   the tracks by DIAG X'20' and writes the label, the directory and the
   allocation map exactly as `edf.py format` does.
8. **I/O.** DMSEDF does its own block I/O by DIAG X'20': seek, search ID,
   TIC, read/write 4096. DMSDIO is untouched, and so is CP (DIAG X'18'
   allows at most 2048 bytes a CCW).

## Order

- **M6b** (read-only ACCESS, LISTFILE, STATE, TYPE) comes first: a disk
  written by `edf.py put` can be read in CMS.
- **M6a** (FORMAT) and **M6c** (write, erase, rename) follow.

Test disk: a 3380 at real 0180 (`disks/edf1.ckd`, uncompressed, in DMKRIO's
180–18F range), attached to the test user.
