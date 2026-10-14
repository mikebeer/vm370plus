#!/bin/sh
# mkjava-debian.sh -- Java for the 31-bit Debian guest (VM/370plus 9.34):
# JamVM 2.0 (xranby/jamvm, our s390 port: jamvm-s390.patch) with GNU
# Classpath 0.99 (gnu-mirror-unofficial/classpath), libffi 3.4.6 and zlib
# 1.3.1 static, cross-built with wcc; glibj.zip and JamVM's classes.zip are
# built on the PC with javac -source 8 -target 8 (byte code is portable).
# NetRexx 3.05 (RexxLA/netrexx tag v3.05) is translated on the PC by its own
# bootstrap compiler running on a PC build of the same JamVM + Classpath
# (it reads the boot class path, so it cannot run on JDK 9+), and compiled
# with javac against glibj.zip.
#   sh mkjava-debian.sh [workdir]  ->  workdir/s390/jamvm-netrexx-s390.tgz
# The steps as they were run on 14 Oct (see README.md for each one):
#   1. classpath: autogen; configure (JAVAC=javac, GCJ_JAVAC_TRUE='#',
#      PATH with javah.py as javah: JDK 10+ has no javah); make -> glibj.zip;
#      make -C include, make -C native (PC) -> PC natives
#   2. jamvm (PC): configure --with-classpath-install-dir=PC prefix;
#      make JAVAC="javac -source 8 -target 8"
#   3. NetRexx 3.05: translate netrexx/lang, then the 11 base classes of
#      org/netrexx/process, then the rest (-nocompile -keepasjava) with the
#      bootstrap NetRexxC.jar on PC JamVM; javac -bootclasspath glibj.zip
#   4. s390: libffi + zlib static with wcc; classpath configure --host
#      s390-linux-gnu, headers from the PC build, make -C native; jamvm
#      configure --host s390-linux-gnu --enable-ffi --disable-int-inlining
#      --disable-shared; make install DESTDIR=stage; wrappers java, nrc, nr
echo "see the comments: the build is recorded step by step, not yet scripted"
