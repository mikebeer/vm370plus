#!/usr/bin/env python3
"""mk390.py -- run.c (karpathy/llama2.c) for VM/370plus (9.17): run390.c.

The checkpoint and the tokenizer are little-endian; Linux/390 and CMS are
big-endian.  run390.c reads the whole checkpoint into storage (no mmap: CMS
has none, and the bytes must be turned anyway) and swaps every 32-bit word
on a big-endian machine; the tokenizer's ints and floats likewise.  With
-DCMS the POSIX includes go."""
import sys
s = open(sys.argv[1] if len(sys.argv) > 1 else 'run.c').read()
def rep(a, b):
    global s
    assert s.count(a) == 1, a[:60]
    s = s.replace(a, b)
rep('''#if defined _WIN32
    #include "win.h"
#else
    #include <unistd.h>
    #include <sys/mman.h>
#endif''', '''#if defined _WIN32
    #include "win.h"
#elif !defined CMS
    #include <unistd.h>
#endif
#include <stdint.h>
/* VM/370plus (9.17): the files are little-endian */
static int big_endian(void) { const uint32_t one = 1; return *(const unsigned char *)&one == 0; }
static void swap32(void *p, size_t n)
{
    unsigned char *b = p, t;
    for (size_t i = 0; i < n; i++, b += 4) {
        t = b[0]; b[0] = b[3]; b[3] = t;
        t = b[1]; b[1] = b[2]; b[2] = t;
    }
}''')
rep('''    FILE *file = fopen(checkpoint, "rb");
    if (!file) { fprintf(stderr, "Couldn't open file %s\\n", checkpoint); exit(EXIT_FAILURE); }
    // read in the config header
    if (fread(config, sizeof(Config), 1, file) != 1) { exit(EXIT_FAILURE); }
    // negative vocab size is hacky way of signaling unshared weights. bit yikes.
    int shared_weights = config->vocab_size > 0 ? 1 : 0;
    config->vocab_size = abs(config->vocab_size);
    // figure out the file size
    fseek(file, 0, SEEK_END); // move file pointer to end of file
    *file_size = ftell(file); // get the file size, in bytes
    fclose(file);
    // memory map the Transformer weights into the data pointer
    *fd = open(checkpoint, O_RDONLY); // open in read only mode
    if (*fd == -1) { fprintf(stderr, "open failed!\\n"); exit(EXIT_FAILURE); }
    *data = mmap(NULL, *file_size, PROT_READ, MAP_PRIVATE, *fd, 0);
    if (*data == MAP_FAILED) { fprintf(stderr, "mmap failed!\\n"); exit(EXIT_FAILURE); }
    float* weights_ptr = *data + sizeof(Config)/sizeof(float);''',
'''    FILE *file = fopen(checkpoint, "rb");
    if (!file) { fprintf(stderr, "Couldn't open file %s\\n", checkpoint); exit(EXIT_FAILURE); }
    // VM/370plus: the whole file into storage (no mmap), in 1 MB reads
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    *data = malloc(*file_size);
    if (!*data) { fprintf(stderr, "no storage for %ld bytes of %s\\n", (long)*file_size, checkpoint); exit(EXIT_FAILURE); }
    for (ssize_t got = 0; got < *file_size; ) {
        size_t want = *file_size - got > (1 << 20) ? (1 << 20) : (size_t)(*file_size - got);
        size_t n = fread((char *)*data + got, 1, want, file);
        if (n == 0) { fprintf(stderr, "short read of %s at %ld\\n", checkpoint, (long)got); exit(EXIT_FAILURE); }
        got += n;
    }
    fclose(file);
    *fd = -1;
    if (big_endian()) swap32(*data, *file_size / 4);   // the ints of Config and every float
    memcpy(config, *data, sizeof(Config));
    // negative vocab size is hacky way of signaling unshared weights. bit yikes.
    int shared_weights = config->vocab_size > 0 ? 1 : 0;
    config->vocab_size = abs(config->vocab_size);
    float* weights_ptr = *data + sizeof(Config)/sizeof(float);''')
rep('''    if (t->data != MAP_FAILED) { munmap(t->data, t->file_size); }
    if (t->fd != -1) { close(t->fd); }''', '''    free(t->data);''')
rep('''    if (fread(&t->max_token_length, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\\n"); exit(EXIT_FAILURE); }''',
'''    if (fread(&t->max_token_length, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\\n"); exit(EXIT_FAILURE); }
    if (big_endian()) swap32(&t->max_token_length, 1);''')
rep('''        if (fread(&len, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\\n"); exit(EXIT_FAILURE); }''',
'''        if (fread(&len, sizeof(int), 1, file) != 1) { fprintf(stderr, "failed read\\n"); exit(EXIT_FAILURE); }
        if (big_endian()) { swap32(t->vocab_scores + i, 1); swap32(&len, 1); }''')
rep('''    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;''', '''#ifdef CMS
    struct timeval tv;              /* CMS (cmsrt): gettimeofday */
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
#endif''')
rep('#include <stdint.h>\n', '#include <stdint.h>\n#ifdef CMS\n#include <sys/time.h>\n#endif\n')
open(sys.argv[2] if len(sys.argv) > 2 else 'run390.c', 'w').write(s)
