/* vmmsg -- VM/370plus 9.14: CP MSG userid text, for any Linux user.
 * Installed setuid root (vmcp needs root).  It runs only "MSG", checks
 * the user ID, strips control characters and starts the text with the
 * Linux user's name, so a message cannot pretend to come from root.
 *   vmmsg userid word ...                                               */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
int main(int argc, char **argv)
{
    char uid[9], text[200], *p;
    struct passwd *pw = getpwuid(getuid());
    int i, n;
    if (argc < 3) { fprintf(stderr, "usage: vmmsg userid message\n"); return 4; }
    n = strlen(argv[1]);
    if (n < 1 || n > 8) { fprintf(stderr, "vmmsg: bad user ID\n"); return 4; }
    for (i = 0; i < n; i++) {
        int c = toupper((unsigned char) argv[1][i]);
        if (!isalnum(c) && c != '$' && c != '#' && c != '@' && c != '*') {
            fprintf(stderr, "vmmsg: bad user ID\n"); return 4;
        }
        uid[i] = c;
    }
    uid[n] = 0;
    snprintf(text, sizeof text, "(%s)", pw ? pw->pw_name : "?");
    for (i = 2; i < argc; i++) {
        strncat(text, " ", sizeof text - strlen(text) - 1);
        strncat(text, argv[i], sizeof text - strlen(text) - 1);
    }
    for (p = text; *p; p++) if ((unsigned char) *p < ' ') *p = ' ';
    {
        char *av[] = { "vmcp", "-k", "MSG", uid, text, NULL };
        char *ev[] = { "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
        execve("/sbin/vmcp", av, ev);
        execve("/usr/sbin/vmcp", av, ev);
    }
    perror("vmmsg: vmcp");
    return 8;
}
