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

The real model, stories15M.bin (60 MB, huggingface.co/karpathy/tinyllamas),
is not in the kit (the build machine cannot reach Hugging Face): on Debian
put it in `/usr/local/share/llama2c/`; on CMS (IPL 290, DEF STOR 128M) as
STORIES BIN on an EDF disk (it has more than 65,533 80-byte records), e.g.
`vmspool.py stories15M.bin cmsuser stories bin --binary`, READCARD.
