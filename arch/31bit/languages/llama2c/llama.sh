#!/bin/sh
# llama -- a tiny language model on Debian/390 (VM/370plus 9.17)
#   llama [prompt ...]     options in the environment: MODEL, TEMP, STEPS, SEED
# llama2c is Karpathy's run.c for big-endian 31-bit (mk390.py).
D=/usr/local/share/llama2c
M=${MODEL:-$D/stories15M.bin}
[ -r "$M" ] || { echo "llama: no model $M (put stories15M.bin from huggingface.co/karpathy/tinyllamas there, or MODEL=file)"; exit 4; }
set -- -z $D/tokenizer.bin -t ${TEMP:-0.8} -n ${STEPS:-256} ${SEED:+-s $SEED} ${*:+-i "$*"}
exec /usr/local/bin/llama2c "$M" "$@"
