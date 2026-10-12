/* xltest.c -- the XL loader's test: code and data above 16 MB */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static char big[24 << 20];                     /* BSS: 24 MB */
static const char msg[] = "XLTEST: data above 16 MB";
int main(int argc, char **argv)
{
    unsigned long sum = 0;
    int i;
    printf("XLTEST: main at %08X, big at %08X, %d args:", (unsigned)main, (unsigned)big, argc);
    for (i = 1; i < argc; i++) printf(" %s", argv[i]);
    printf("\n%s\n", msg);
    for (i = 0; i < (24 << 20); i += 4096) big[i] = (char)(i >> 12);
    for (i = 0; i < (24 << 20); i += 4096) sum += (unsigned char)big[i];
    char *h = malloc(32 << 20);
    printf("XLTEST: BSS sum %lu, heap 32 MB at %08X\n", sum, (unsigned)h);
    FILE *f = fopen("xltest.out", "w");
    fprintf(f, "written by XLTEST from high storage\n");
    fclose(f);
    f = fopen("xltest.out", "r");
    char line[80];
    fgets(line, sizeof line, f);
    fclose(f);
    printf("XLTEST: read back: %s", line);
    return 7;
}
