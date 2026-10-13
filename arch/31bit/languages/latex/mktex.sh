#!/bin/sh
# mktex.sh -- TeX Live 2023 for the 31-bit Debian guest (VM/370plus 9.27):
# pdfTeX (latex, pdflatex), dvips, kpsewhich, makeindex, cross-built with
# wcc/wc++ (GCC 13 -m31, wheezy glibc), then a small texmf tree and the
# LaTeX formats.  Formats are portable: they are made on the PC with the
# PC's pdfTeX 1.40.25 (the same version) over the same tree and texmf.cnf;
# making them in the guest takes over half an hour.
#   sh mktex.sh [workdir]   ->  workdir/texroot.tgz  (unpack at / in the guest)
set -e
W=${1:-/home/claude/src/tex}; HERE=$(cd $(dirname $0) && pwd); L=$HERE/../common
P=$W/s390root
mkdir -p $W && cd $W
ls texlive-bin_*.orig.tar.* >/dev/null 2>&1 || apt-get source --download-only texlive-bin
[ -d texlive-bin-2023* ] || tar xf texlive-bin_*.orig.tar.*
# zlib and libpng for the target (sources as for Ghostscript)
GS=${GSW:-/home/claude/src/gs}
[ -f $P/lib/libz.a ] || (rm -rf zb && cp -r $GS/zlib-1.3.dfsg zb && cd zb && CC=$L/wcc ./configure --static --prefix=$P && make && make install)
[ -f $P/lib/libpng16.a ] || (rm -rf pb && cp -r $GS/libpng-1.6.43 pb && cd pb && CC=$L/wcc CPPFLAGS=-I$P/include LDFLAGS=-L$P/lib ./configure --host=s390-linux-gnu --disable-shared --prefix=$P && make && make install)
install -m 755 $HERE/pkg-config $P/pkg-config
sed -i "s|/home/claude/src/tex/s390root|$P|g" $P/pkg-config
rm -rf build && mkdir build && cd build
PKG_CONFIG=$P/pkg-config CC=$L/wcc CXX=$L/wc++ BUILDCC=gcc BUILDCXX=g++ \
CPPFLAGS="-I$P/include" LDFLAGS="-L$P/lib" LIBS=-lm \
../texlive-bin-2023*/configure --host=s390-linux-gnu --build=x86_64-linux-gnu \
  --disable-native-texlive-build --disable-all-pkgs --enable-web2c --enable-pdftex \
  --enable-dvipsk --enable-makeindexk --disable-luatex --disable-luajittex \
  --disable-luahbtex --disable-luajithbtex --disable-mflua --disable-mfluajit \
  --disable-xetex --disable-ptex --disable-eptex --disable-uptex --disable-euptex \
  --disable-aleph --disable-hitex --disable-mf --disable-mp --disable-pmp --disable-upmp \
  --disable-synctex --disable-shared --without-x --with-system-zlib --with-system-libpng \
  --prefix=/usr/local/texlive
make -j4 -C libs && make -j4 -C texk     # (utils/ does not configure; not needed)
# the tree: what LaTeX, amsmath, graphicx, tools, geometry and dvips need
R=$W/texroot/usr/local/texlive; rm -rf $W/texroot; mkdir -p $R/bin $R/texmf-var/web2c
for f in web2c/pdftex dvipsk/dvips kpathsea/kpsewhich makeindexk/makeindex; do
  s390x-linux-gnu-strip -o $R/bin/$(basename $f) texk/$f; done
cp $HERE/bin/* $R/bin/
D=/usr/share/texlive/texmf-dist          # apt install texlive-latex-base on the PC
for d in tex/latex/base tex/latex/l3kernel tex/latex/l3backend tex/latex/amsmath \
  tex/latex/amscls tex/latex/graphics tex/latex/graphics-cfg tex/latex/graphics-def \
  tex/latex/tools tex/latex/latexconfig tex/latex/firstaid tex/latex/url tex/latex/geometry \
  tex/latex/l3packages tex/latex/tex-ini-files tex/generic/hyphen tex/generic/unicode-data \
  tex/generic/babel tex/generic/tex-ini-files tex/generic/pdftex fonts/tfm/public/cm \
  fonts/tfm/public/amsfonts fonts/tfm/public/latex-fonts fonts/tfm/jknappen/ec \
  fonts/type1/public/amsfonts dvips/base dvips/l3backend dvips/config makeindex; do
  [ -e $D/$d ] && { mkdir -p $R/texmf-dist/$(dirname $d); cp -r $D/$d $R/texmf-dist/$d; }; done
mkdir -p $R/texmf-dist/tex/generic/config $R/texmf-dist/web2c \
  $R/texmf-dist/fonts/map/dvips/updmap $R/texmf-dist/fonts/map/pdftex/updmap
cp $HERE/language.dat $HERE/language.def $D/tex/generic/config/pdftex-dvi.tex $R/texmf-dist/tex/generic/config/
cp /var/lib/texmf/fonts/map/dvips/updmap/psfonts*.map $R/texmf-dist/fonts/map/dvips/updmap/
cp /var/lib/texmf/fonts/map/pdftex/updmap/pdftex.map $R/texmf-dist/fonts/map/pdftex/updmap/
cp $HERE/texmf.cnf $D/web2c/cp227.tcx $R/texmf-dist/web2c/
# the formats, on the PC
F=$W/pcfmt; rm -rf $F; mkdir -p $F
sed "s|^TEXMFROOT = /usr/local/texlive|TEXMFROOT = $R|" $HERE/texmf.cnf > $F/texmf.cnf
(cd $F && for j in latex pdflatex; do TEXMFCNF=$F pdftex -ini -etex -jobname=$j -progname=$j \
   -translate-file=cp227.tcx -interaction=nonstopmode "*$j.ini" >$j.out 2>&1; done)
cp $F/latex.fmt $F/pdflatex.fmt $R/texmf-var/web2c/
cd $W/texroot && tar czf $W/texroot.tgz usr && ls -la $W/texroot.tgz
