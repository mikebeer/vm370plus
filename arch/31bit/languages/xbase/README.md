# xBase for cREXX

An interpreter for the xBase language family (Clipper, dBASE, [Harbour](https://github.com/harbour/core))
written in [cREXX](https://github.com/adesutherland/CREXX) Level B. It reads `.prg` source files and runs them,
with the language, `.dbf` tables with indexes, and console input and output.

```
$ cat hello.prg
PROCEDURE Main()
   ? "Hello from xBase on cREXX!"
RETURN
$ ./xbase hello.prg
Hello from xBase on cREXX!
```

## Build and run

You need a cREXX system (1.0.0 beta 3 or later) with `rxc`, `rxas` and `rxvm`. If they are not on the
`PATH`, set `CREXX_BIN` to their directory.

```
make            # builds build/xbase.rxbin from xbase.crexx
./xbase prog.prg [args]
make test       # runs tests/*.prg and compares with tests/*.out
```

`xbase.crexx` is generated from `src/` by `src/gen.py` (`make gen`, needs Python 3); it is committed so that
building needs only cREXX.

## What it understands

* **Language**: `FUNCTION`/`PROCEDURE`/`STATIC`/`INIT`/`EXIT`, `LOCAL`/`STATIC`/`PRIVATE`/`PUBLIC`/`MEMVAR`,
  `IF`, `DO WHILE`, `FOR`, `FOR EACH`, `DO CASE`, `SWITCH`, `BEGIN SEQUENCE`/`RECOVER`, `TRY`/`CATCH`,
  codeblocks with closures, arrays, hashes, `&` macros, `#define`/`#include`/`#ifdef`,
  classes (`CREATE CLASS`, `VAR`, `METHOD`, `INHERIT`, `::Parent:Method()`).
* **Data types**: character, numeric, date, logical, NIL, array, hash, codeblock, object.
* **Functions**: about 210 built-ins (strings, maths, dates, arrays, hashes, files, error handling, `Transform`).
* **Databases**: `.dbf` (dBASE III) tables with `C N D L M` fields, up to 40 work areas, `USE`, `APPEND`,
  `REPLACE`, `DELETE`/`RECALL`/`PACK`/`ZAP`, `GO`/`SKIP`/`SEEK`/`LOCATE`/`CONTINUE`, `COUNT`/`SUM`/`AVERAGE`,
  `LIST`/`DISPLAY`, `INDEX ON` (several per table, `UNIQUE`, `DESCENDING`, `FOR`), `SET ORDER/FILTER/RELATION/DELETED`,
  and the `dbXxx()` function family.
* **Console**: `?`, `??`, `@ SAY/GET`, `READ`, `ACCEPT`, `INPUT`, `WAIT`, `KEYBOARD`, `SET`.

The manual is in [`docs/XBase-User-Guide.docx`](docs/XBase-User-Guide.docx).

## Restrictions

* The console is line oriented: `@ row,col` writes in sequence (new row = new line) and `READ` reads one line
  per `GET`. There is no full-screen cursor addressing, colour, or keyboard polling.
* Indexes are kept in memory and saved to a private `.xdx` text file; NTX/CDX/MDX files are not read or written.
* Tables are loaded into memory on `USE` (at most 100000 records each) and written back on `CLOSE`, `COMMIT`
  or exit. There is no record or file locking.
* Memo fields (`M`) are stored in a `.dbt` file in dBASE III style.
* cREXX cannot delete a file, so `ERASE` and `FErase()` truncate it; an empty file counts as absent.
* No garbage collection; long-running programs that build many strings or arrays use more memory.
* Not supported: `SORT`, `UPDATE`, `JOIN`, `TOTAL`, `SAVE`/`RESTORE`, `#command`/`#translate`, threads,
  `hb_*` functions beyond those listed in the manual.

## Layout

| Path | What |
|---|---|
| `xbase` | launcher script |
| `xbase.crexx` | the interpreter (generated) |
| `src/` | the sources, `gen.py` and the function tables `lib*.tpl` |
| `tests/` | test programs, expected output (`.out`) and input (`.in`) |
| `examples/` | sample programs |
| `docs/` | the manual |

## VM/370plus (9.6)

CMS: `XBASE fn.prg [args]` runs XBASE RXBIN on RXBVM8 (the cREXX VM
cross-built for CMS). Test tc37 (12 October 2026): all 7 tests in
`tests/` give the PC's output on CMS (`cms/XBTEST EXEC` runs them, input
from `fn IN` through the stack). Tables (`.dbf`), memos (`.dbt`) and
index files (`.xdx`) are binary CMS files (filetypes DBF, DBT, XDX);
the index was read as text before upstream 3ca444f, which on CMS meant
EBCDIC. Debian: `xbase fn.prg` (cREXX bundle).
