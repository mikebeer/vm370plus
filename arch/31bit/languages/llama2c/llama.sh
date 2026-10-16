#!/bin/sh
# llama -- a tiny language model on Debian/390 (VM/370plus 9.17)
#   llama [prompt ...]     options in the environment: MODEL, TEMP, STEPS, SEED
#   llama --get [URL]      fetch the model (URL and MD5 in /etc/llama.conf)
# llama2c is Karpathy's run.c for big-endian 31-bit (mk390.py).  The model
# (stories15M, 58 MB) wants a machine of 192 MB (DEF STOR 192M).
D=/usr/local/share/llama2c
M=${MODEL:-$D/stories15M.bin}
[ -r /etc/llama.conf ] && . /etc/llama.conf
if [ "$1" = --get ]; then
  U=${2:-$URL}; T=$D/.get.$$
  [ -n "$U" ] || { echo "llama: no URL (/etc/llama.conf)"; exit 4; }
  echo "llama: fetching $U"
  if command -v wget >/dev/null; then wget -q --no-check-certificate -O $T "$U"
  elif command -v curl >/dev/null; then curl -s -k -o $T "$U"
  else python3 -c "import sys, ssl, urllib.request
open(sys.argv[2], 'wb').write(urllib.request.urlopen(sys.argv[1], timeout=600).read())" "$U" $T
  fi || { rm -f $T; echo "llama: the download failed (does this machine reach the internet? see the Guide)"; exit 8; }
  if [ -n "$MD5" ] && [ "$(md5sum < $T | cut -d' ' -f1)" != "$MD5" ]; then
    rm -f $T; echo "llama: the file is not the expected model (MD5)"; exit 8
  fi
  mv $T $D/stories15M.bin && echo "llama: $D/stories15M.bin, $(wc -c < $D/stories15M.bin) bytes"
  exit 0
fi
[ -r "$M" ] || { echo "llama: no model $M -- llama --get fetches it"; exit 4; }
set -- -z $D/tokenizer.bin -t ${TEMP:-0.8} -n ${STEPS:-256} ${SEED:+-s $SEED} ${*:+-i "$*"}
exec /usr/local/bin/llama2c "$M" "$@"
