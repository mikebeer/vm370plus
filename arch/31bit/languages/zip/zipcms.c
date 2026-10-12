/* zipcms.c -- what Info-ZIP's ttyio.c and unix.c expect from the system,
   for CMS (VM/370plus 9.23): no terminal modes, no file owners. */
#include <stdio.h>
#include <sys/types.h>
int chown(const char *p, uid_t u, gid_t g) { (void)p; (void)u; (void)g; return 0; }
#ifdef UNZIP_STUBS
struct Globals;
void Echon(void *pG) { (void)pG; }
void Echoff(void *pG, int f) { (void)pG; (void)f; }
int screensize(int *tt_rows, int *tt_cols)
{
    if (tt_rows) *tt_rows = 24;
    if (tt_cols) *tt_cols = 80;
    return 0;
}
int zgetch(void *pG, int f) { int c; (void)pG; (void)f; c = getchar(); return c == EOF ? 0 : c; }
#endif
