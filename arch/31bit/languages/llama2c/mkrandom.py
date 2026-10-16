#!/usr/bin/env python3
"""mkrandom.py OUT [SEED [DIM]] -- a llama2.c checkpoint of random weights (dim 64,
2 layers, the 32000-token vocabulary of tokenizer.bin): no use as a model,
but it proves a port -- the same file must give the same tokens on the PC,
on Linux/390 and on CMS (9.17)."""
import struct, sys
import numpy as np
dim = int(sys.argv[3]) if len(sys.argv) > 3 else 64
hidden, layers, heads, kv, vocab, seq = dim * 8 // 3, 2, 4, 4, 32000, 128
hs = dim // heads
rng = np.random.default_rng(int(sys.argv[2]) if len(sys.argv) > 2 else 370)
def r(*shape): return (rng.standard_normal(shape) * 0.08).astype('<f4')
parts = [r(vocab, dim), np.ones((layers, dim), '<f4'), r(layers, dim, dim), r(layers, dim, kv * hs),
         r(layers, dim, kv * hs), r(layers, dim, dim), np.ones((layers, dim), '<f4'),
         r(layers, hidden, dim), r(layers, dim, hidden), r(layers, hidden, dim), np.ones(dim, '<f4'),
         np.zeros(seq * hs // 2, '<f4'), np.zeros(seq * hs // 2, '<f4')]
with open(sys.argv[1], 'wb') as f:
    f.write(struct.pack('<7i', dim, hidden, layers, heads, kv, vocab, seq))
    for p in parts: f.write(p.tobytes())
