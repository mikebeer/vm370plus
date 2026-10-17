# CPWATCH rebuilt for VM/370plus (4c, I-251)

`orig/` holds the sources as punched from CPWATCH's 191 (with spool
separator banners). Changes for VM/370plus, kept here:

- `CPWR6.CNTRL`: `MACS CPW DMKLCL DMKMAC` -- assemble against our own
  macro library first, so `RDEVBLOK`/`RDEVSIZE` are VM/370plus's (the
  2005 TEXT files walked RDEVBLOKs with CE's old stride; I-251).
- `CPWSUP.ASSEMBLE`: its label `HOLDSCR` clashed with CE's
  `HOLDSCR EQU X'2000'` (HRC101DK, EQU COPY); renamed `CPWHOLD`.

Rebuild: `tests/runs/cpwatch-rebuild.json` (CPWATCH's own disk, `ACCESS
(NOPROF`, MAINT 191 as C for DMKLCL MACLIB, then `EXEC ASMALL`).
- 31-bit CP addresses (VM/370plus puts control blocks above 16 MB):
  `CPWSUP` cleaned every CP address with `LA R5,0(,R5)` (24 bits in its
  S/370 BC machine) -- now `N R5,=X'7FFFFFFF'`; `CPWUSR` called a VMBLOK
  pointer with a non-zero first byte a chain failure ("VMBLOK CHAIN
  FAILURE" without end, cw2) -- now only the X'80' bit is.
