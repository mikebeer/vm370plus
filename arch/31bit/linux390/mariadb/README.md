# MariaDB 10.11 in the Debian guest (9.47)

MariaDB 10.11.14 cross-built for the 31-bit Debian 7 guest (`mkmariadb.sh`:
wcc/wc++, GCC 13 -m31 against wheezy's glibc 2.13, from Ubuntu's source
package). It is the database of MCproject (9.36) and MChelpdesk (9.38),
both Regina REXX servers that talk to MariaDB through the `mariadb`
command-line client, so they run on it unchanged.

Installed under `/usr/local/mariadb`, data in `/var/lib/mariadb`, socket
`/var/run/mysqld/mysqld.sock`, started at boot (`/etc/init.d/mariadb`).
Settings for a 64 MB machine that also runs PostgreSQL (`my.cnf`, installed
as `/etc/my.cnf`): 10 connections, 8 MB InnoDB buffer pool, 1 MB key and
Aria caches, no query cache; local connections only (127.0.0.1 and the
socket). `root@localhost` logs in through the socket as on current Debian:
as root, just `mariadb`.

What is built: the server with InnoDB, Aria, MyISAM, MEMORY, CSV,
Sequence; the client tools (`mariadb`, `mariadb-admin`, `mariadb-dump`,
...). Not built: performance_schema, Galera, RocksDB, Mroonga, Spider,
CONNECT, S3, OQGRAPH, PAM and GSSAPI authentication, the compression
providers. The server uses the bundled WolfSSL; the client library has no
TLS (it only talks to the local server).

Install: `sh install.sh` in a directory with `mariadb-s390.tgz`, `my.cnf`
and `mariadb.init`.

## Build notes

- Host tools first: a native build's `import_executables` (comp_err,
  gen_lex_hash, ...), then the cross build with `-DIMPORT_EXECUTABLES`.
- `s390-wheezy.cmake` is the toolchain file; `MDB_TGT` holds the libraries
  cross-built here (static ncurses 6.4 for the client's readline).
- `no-htm.patch`: InnoDB uses s390 hardware transactional memory only when
  the compiler targets zEC12 or later (`__HTM__`); our default target does not.
- Not PIE (`SECURITY_HARDENED=OFF`).
- I-300: the first build died at its first C++ global because wc++ linked
  the cross toolchain's glibc 2.39 `crt1.o`; wcc and wc++ now link wheezy's
  own start files.
