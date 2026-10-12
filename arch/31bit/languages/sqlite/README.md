# SQLite on CMS (9.4)

    SQLITE fn.db                 the sqlite3 shell on FN DB (end: .quit)
    SQLITE fn.db sql             one SQL statement or .command, e.g.
    SQLITE fn.db .read fn.sql    a script (FN SQL)

SQLite 3.46.1 (amalgamation and shell, from Ubuntu's source package)
cross-built with `m5f/cmscc` by `mksqlite-cms.sh`: SQLITE3 TEXT, LOAD and
GENMOD as SQLITE3 MODULE (1.7 MB, below 16 MB; its heap is above).

The Unix VFS runs on cmsrt's POSIX layer:
- a database is a binary CMS file (filetype DB, records of up to 32 KB);
  its rollback journal is `fn DB-JOURN`;
- `fsync` writes the CMS file (cmsrt otherwise writes at close);
- locks are always granted: one user per disk, no threads, no WAL, no mmap;
  temporary tables and sorts stay in storage (`SQLITE_TEMP_STORE=3`).

What cmsrt needed (12 October 2026): lseek past the end no longer grows
the file (POSIX: only a write does); `stat` of a file the program has open
for writing describes that file (SQLite checks that its new database was
not unlinked); `st_nlink` 1; `/fn.ft` (one component) is a CMS file, since
SQLite makes names absolute against `getcwd()` = "."; `ftruncate`, F_GETLK
answers "unlocked", stubs for the user and process calls.

Test tc31: `tests/sqltest.sql` (table, index, GROUP BY, transaction with
UPDATE/DELETE, printf, recursive CTE, randomblob) gives `tests/sqltest.pc`,
the PC's output, line for line; `sqltest2.sql` reopens the database
(rows, sum, `pragma integrity_check` = ok).
