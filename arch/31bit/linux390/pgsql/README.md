# PostgreSQL 16 in the Debian guest, and PGSQL for CMS (9.4)

PostgreSQL 16.15 cross-built for the 31-bit Debian guest (`mkpg.sh`: wcc,
GCC 13 -m31 against wheezy's glibc 2.13; Ubuntu's source package; no
readline, zlib, ICU, OpenSSL), installed under `/usr/local/pgsql`, data in
`/var/lib/pgsql16`, started at boot (`/etc/init.d/pgsql16`). Settings for a
64 MB machine: 10 connections, 4 MB shared buffers. Local connections only,
trusted.

On CMS, `PGSQL` sends SQL over the spool bridge (LXCALL, service `pgsql`):

    PGSQL create table parts (no int, name text)
    PGSQL insert into parts values (1, 'widget')
    PGSQL select * from parts
    PGSQL (FILE QUERY SQL         the statements of QUERY SQL A

Each CMS user gets a role and a database named after the user ID (lower
case) on first use. Linux users: `psql -U postgres`.

Install: `sh install.sh` in a directory with `pgsql16-s390.tgz`,
`pgsql.init` and `pgsql.handler`.
