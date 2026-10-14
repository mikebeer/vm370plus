#!/bin/sh
# install.sh -- PostgreSQL 16 on the Debian guest (VM/370plus 9.4), as root,
# in this directory with pgsql16-s390.tgz (mkpg.sh).  Small settings for a
# 64 MB machine; local connections only, trusted (the spool bridge maps
# each CMS user to a role of its own name).
set -e
tar xzf pgsql16-s390.tgz -C /
# the Debian disk is small: no headers and no tools the bridge does not use
rm -rf /usr/local/pgsql/include
(cd /usr/local/pgsql/bin && rm -f ecpg pg_upgrade pgbench pg_basebackup pg_receivewal \
  pg_recvlogical pg_rewind pg_verifybackup pg_waldump pg_amcheck pg_test_fsync \
  pg_test_timing pg_checksums pg_archivecleanup)
id postgres >/dev/null 2>&1 || useradd -r -d /var/lib/pgsql16 -s /bin/sh postgres
D=/var/lib/pgsql16
if [ ! -s $D/PG_VERSION ]; then
  mkdir -p $D && chown postgres $D
  su postgres -c "/usr/local/pgsql/bin/initdb -D $D -U postgres -A trust -E UTF8 --locale=C --wal-segsize=1" >/dev/null
  cat >> $D/postgresql.conf <<'C'
# VM/370plus: a 64 MB virtual machine
listen_addresses = 'localhost'
max_connections = 10
shared_buffers = 4MB
work_mem = 1MB
maintenance_work_mem = 4MB
dynamic_shared_memory_type = posix
max_wal_size = 8MB
min_wal_size = 2MB
C
fi
install -m 755 pgsql.init /etc/init.d/pgsql16
update-rc.d pgsql16 defaults >/dev/null
[ -d /etc/vmspool.d ] && install -m 755 pgsql.handler /etc/vmspool.d/pgsql
ln -sf /usr/local/pgsql/bin/psql /usr/local/bin/psql
/etc/init.d/pgsql16 start
/usr/local/pgsql/bin/psql -X -U postgres -c 'select version()'
