#!/bin/sh
# install.sh -- MariaDB 10.11 on the Debian guest (VM/370plus 9.47), as
# root, in this directory with mariadb-s390.tgz (mkmariadb.sh), my.cnf and
# mariadb.init.  root@localhost logs in through the socket (unix_socket),
# as on current Debian.
set -e
tar xzf mariadb-s390.tgz -C /
id mysql >/dev/null 2>&1 || useradd -r -d /var/lib/mariadb -s /bin/false mysql
install -m 644 my.cnf /etc/my.cnf
mkdir -p /var/run/mysqld && chown mysql /var/run/mysqld
D=/var/lib/mariadb
if [ ! -d $D/mysql ]; then
  mkdir -p $D && chown mysql $D
  /usr/local/mariadb/scripts/mariadb-install-db --defaults-file=/etc/my.cnf \
    --basedir=/usr/local/mariadb --datadir=$D --user=mysql --skip-test-db >/dev/null
fi
install -m 755 mariadb.init /etc/init.d/mariadb
update-rc.d mariadb defaults >/dev/null
for p in mariadb mariadb-admin mariadb-dump mysql mysqladmin mysqldump; do
  ln -sf /usr/local/mariadb/bin/$p /usr/local/bin/$p; done
/etc/init.d/mariadb start
/usr/local/mariadb/bin/mariadb -e 'select version(), @@innodb_buffer_pool_size'
