/* pcintcms.c -- PCINT (the Stanford Pascal P-code interpreter) on CMS
 * (VM/370plus 9.13a).  PCINT finds its files through environment
 * variables DD_name, as FILEDEF would; on CMS the EXECs pass them as
 * arguments "DD_name=fn.ft" (through the stack), which become the
 * environment here before PCINT's own main runs. */
#include <stdlib.h>
#include <string.h>
int pcint_main(int argc, char **argv);
int main(int argc, char **argv)
{
    int n = 1;
    for (int i = 1; i < argc; i++) {
        char *eq = strchr(argv[i], '=');
        if (!strncmp(argv[i], "DD_", 3) && eq) {
            *eq = 0;
            setenv(argv[i], eq + 1, 1);
        } else
            argv[n++] = argv[i];
    }
    argv[n] = 0;
    return pcint_main(n, argv);
}
