# llama2.c on VM/370plus (9.17)

Karpathy's [llama2.c](https://github.com/karpathy/llama2.c) `run.c` (MIT,
`LICENSE`), inference of Llama-2-architecture models in one C file, for the
two big-endian 31-bit systems of VM/370plus:

- `mk390.py run.c run390.c` -- the port: the checkpoint is read into storage
  (no mmap: CMS has none) and every 32-bit word swapped on a big-endian
  machine; the tokenizer's ints and floats likewise; `gettimeofday` on CMS.
- Debian/390: `../common/wcc -O2 -march=z900 -ffp-contract=off -o run390.deb run390.c -lm -lrt`,
  installed as `/usr/local/bin/llama2c`, `llama.sh` as `/usr/local/bin/llama`,
  `tokenizer.bin` in `/usr/local/share/llama2c/`.
- CMS: `../../m5f/cmscc -O2 -DCMS -ffp-contract=off -o run390.elf run390.c -lm`,
  `elf_to_cms run390.elf run390.text` -> LLAMA2C TEXT on the language disk
  (LANGGEN makes the MODULE), `tokenizer.bin` as LLAMATOK BIN, `LLAMA.EXEC`
  (the front end: options, and the command line through `=STACK`, as CMS cuts
  tokens to 8 characters).  Soft float (the M5f runtime).
- `mkrandom.py OUT [SEED [DIM]]` -- a checkpoint of random weights with the
  32000-token vocabulary: useless as a model, but the same file must give
  the same tokens everywhere.

Verified 16 Oct: `-t 0 -i VM` on a dim-32 random model gives the same 24
tokens ("VMferrerferrer judge judge ...") on the PC (x86-64), on Debian/390
(llmd, 3.0 tok/s) and on CMS (llm1/llm2/llmi, 0.9 tok/s, soft float); the
dim-64 model and seeded sampling (`-t 0.9 -s 42`) likewise.

The model, stories15M.bin (60,816,028 bytes, MD5 644db0bc012b405d6baf99559272ab11,
from huggingface.co/karpathy/tinyllamas), is not in the kit but on
https://vm370plus.com/wp-content/uploads/stories15M.bin (`llama.conf`):
`llama --get` on Debian; `LLAMA GET fm` on CMS -- the service `llamaget`
(linux390/vmspool/handlers) fetches it if need be and has vmspoold punch it
(outbox `FILE`, binary, 760,201 cards, about 14 KB/s) to the caller, LLAMA
EXEC reads it onto an EDF disk.  Verified 16 Oct: on CMS "Once upon a
time, there was a" as on the PC (0.066 tok/s); Debian in 192 MB 0.11 tok/s.
