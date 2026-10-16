/*
 * vmspoold -- VM/370plus 9.25: the spool bridge.  Runs in the Debian guest
 * and turns reader files into calls of Linux services, as SQLSERV does on
 * CMS: a CMS user punches a request to the Linux virtual machine, vmspoold
 * runs the service's handler and punches the answer back.
 *
 *   vmspoold [-r 0.0.000c] [-p 0.0.000d] [-d /etc/vmspool.d]
 *            [-s /var/spool/vmspool] [-i seconds] [-t timeout(900)] [-1] [-v]
 *
 * A request is a punch file whose first card is
 *     service FROM userid [argument ...]
 * and whose other cards are the input of the service.  The handler is the
 * executable  <dir>/<service>  (lower case), run with the arguments, the
 * input on stdin, VMSPOOL_FROM and VMSPOOL_SERVICE in its environment
 * (and VMSPOOL_RAW: a file of the input cards as they came, EBCDIC).
 * The answer, punched to userid as  VMSPOOL REPLY , is
 *     VMSPOOL RC n
 * followed by what the handler wrote (stdout and stderr), cut into
 * 80-column cards.  A reader file whose first card is not a request is
 * kept in <spool>/unknown and not answered.
 *
 * The reader is read through vmur (VM/370 mode: CCW READ, one card per
 * I/O, unit exception = end of file; needs the CP fix of I-276).  A read()
 * that returns less than it asked for, or 0, ends the file.
 *
 * 9.18: files left in <spool>/outq are punched too (see outbox()).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>

static const unsigned char e2a[256] = {
    0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
    0x80,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
    0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
    0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
    0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0x5E,
    0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
    0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
    0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
    0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0xA4,
    0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0x5B,0xDE,0xAE,
    0xAC,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0xDD,0xA8,0xAF,0x5D,0xB4,0xD7,
    0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
    0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
    0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F
};
static unsigned char a2e[256];

static const char *rdrdev = "0.0.000c", *pundev = "0.0.000d";
static const char *hdir = "/etc/vmspool.d", *sdir = "/var/spool/vmspool";
static int interval = 5, tmo = 900, once = 0, verbose = 0;
static char myid[9] = "";       /* this virtual machine's user ID */
static int cp(const char *cmd, char *buf, size_t len);

/* QUERY VIRTUAL of the punch says  "     00D FOR userid ..." */
static void whoami(void)
{
    char c[64], r[512], *p;
    snprintf(c, sizeof c, "q v %s", pundev + 5);
    cp(c, r, sizeof r);
    p = strstr(r, " FOR ");
    if (p) sscanf(p + 5, "%8s", myid);
}

/* a closed file waits in our reader?  QUERY FILES counts the whole
   system for a class D user, and a READ of an empty reader ends in a
   unit check, so ask for our own reader files (as SQLSERV does) */
static int waiting(void)
{
    char c[64], r[2048], *l;
    if (!myid[0]) return 1;
    snprintf(c, sizeof c, "q rdr all %s", myid);
    cp(c, r, sizeof r);
    for (l = strtok(r, "\n"); l; l = strtok(NULL, "\n")) {
        char w1[16], w2[16], w3[16];
        /* only class A: the reader is class A, and files of another
           class (MONDATA from the monitor, class M) cannot be read */
        if (sscanf(l, "%15s %15s %15s", w1, w2, w3) == 3 && isdigit((unsigned char) w2[0])
            && strspn(w2, "0123456789") == strlen(w2) && !strcmp(w3, "A"))
            return 1;
    }
    return 0;
}

#define MAXIN  (4 << 20)        /* a request: 4 MB of cards at most */
#define MAXOUT (1 << 20)        /* an answer: 1 MB */

static void logf_(const char *fmt, const char *a, const char *b)
{
    char ts[32];
    time_t t = time(NULL);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(stderr, "vmspoold %s: ", ts);
    fprintf(stderr, fmt, a, b);
    fputc('\n', stderr);
}

static void online(const char *dev)
{
    char p[96];
    FILE *f;
    snprintf(p, sizeof p, "/sys/bus/ccw/devices/%s/online", dev);
    f = fopen(p, "w");
    if (f) { fputs("1\n", f); fclose(f); }
}

/* CP command by vmcp; its answer into buf (may be NULL) */
static int cp(const char *cmd, char *buf, size_t len)
{
    char line[256];
    FILE *p;
    size_t n = 0;
    int rc;
    snprintf(line, sizeof line, "vmcp %s 2>&1", cmd);
    p = popen(line, "r");
    if (!p) return -1;
    if (buf && len) {
        n = fread(buf, 1, len - 1, p);
        buf[n] = 0;
    } else {
        while (fread(line, 1, sizeof line, p) > 0) ;
    }
    rc = pclose(p);
    if (verbose) logf_("CP %s -> %s", cmd, buf ? buf : "");
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
}

/* one reader file: its bytes (a multiple of 80) or NULL if none */
static unsigned char *readfile(size_t *len)
{
    char p[64];
    unsigned char *buf;
    size_t n = 0, chunk = MAXIN;     /* one read() per file: vmur asks QUERY
                                       FILES before each read, and a file
                                       being read is not counted (I-280) */
    int fd;
    ssize_t r;
    snprintf(p, sizeof p, "/dev/vmrdr-%s", rdrdev);
    fd = open(p, O_RDONLY);
    if (fd < 0) return NULL;
    buf = malloc(MAXIN + chunk);
    if (!buf) { close(fd); return NULL; }
    for (;;) {
        r = read(fd, buf + n, chunk);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        n += r;
        if ((size_t) r < chunk || n >= MAXIN) break;
    }
    close(fd);
    if (n == 0) { free(buf); return NULL; }
    *len = n - n % 80;
    return buf;
}

/* card i of the file as ASCII, trailing blanks stripped */
static void card(const unsigned char *b, size_t i, char *out)
{
    int k, e = 0;
    for (k = 0; k < 80; k++) {
        out[k] = (char) e2a[b[i * 80 + k]];
        if (out[k] != ' ') e = k + 1;
    }
    out[e] = 0;
}

static int valid_user(const char *u)
{
    size_t i, n = strlen(u);
    if (n < 1 || n > 8) return 0;
    for (i = 0; i < n; i++)
        if (!isalnum((unsigned char) u[i]) && !strchr("$#@", u[i])) return 0;
    return 1;
}

static int valid_service(const char *s)
{
    size_t i, n = strlen(s);
    if (n < 1 || n > 16) return 0;
    for (i = 0; i < n; i++)
        if (!isalnum((unsigned char) s[i]) && s[i] != '_' && s[i] != '-') return 0;
    return 1;
}

/* run the handler: input in, its output into out (len in *olen), rc back */
static int run(const char *path, char **argv, const char *from, const char *svc,
               const char *in, size_t inlen, char *out, size_t *olen)
{
    int pin[2], pout[2], status = 0, done = 0;
    pid_t pid;
    size_t w = 0, o = 0;
    time_t end = time(NULL) + tmo;

    if (pipe(pin) < 0 || pipe(pout) < 0) return 255;
    pid = fork();
    if (pid < 0) return 255;
    if (pid == 0) {
        dup2(pin[0], 0); dup2(pout[1], 1); dup2(pout[1], 2);
        close(pin[0]); close(pin[1]); close(pout[0]); close(pout[1]);
        setenv("VMSPOOL_FROM", from, 1);
        setenv("VMSPOOL_SERVICE", svc, 1);
        execv(path, argv);
        printf("vmspoold: cannot run %s: %s\n", path, strerror(errno));
        _exit(127);
    }
    close(pin[0]); close(pout[1]);
    fcntl(pin[1], F_SETFL, O_NONBLOCK);
    if (inlen == 0) { close(pin[1]); pin[1] = -1; }
    while (!done) {
        struct pollfd pf[2];
        int np = 0, r;
        pf[np].fd = pout[0]; pf[np].events = POLLIN; np++;
        if (pin[1] >= 0) { pf[np].fd = pin[1]; pf[np].events = POLLOUT; np++; }
        r = poll(pf, np, 1000);
        if (r < 0 && errno != EINTR) break;
        if (time(NULL) > end) {
            kill(pid, SIGKILL);
            o += snprintf(out + o, MAXOUT - o, "\nvmspoold: %s timed out after %d s\n", svc, tmo);
            break;
        }
        if (r <= 0) continue;
        if (pf[0].revents & (POLLIN | POLLHUP)) {
            ssize_t n = read(pout[0], out + o, MAXOUT - 1 - o);
            if (n <= 0) done = 1;
            else o += n;
            if (o >= MAXOUT - 1) { kill(pid, SIGKILL); done = 1; }
        }
        if (np > 1 && (pf[1].revents & (POLLOUT | POLLERR | POLLHUP))) {
            ssize_t n = write(pin[1], in + w, inlen - w);
            if (n > 0) w += n;
            if (n < 0 && errno != EAGAIN) w = inlen;
            if (w >= inlen) { close(pin[1]); pin[1] = -1; }
        }
    }
    if (pin[1] >= 0) close(pin[1]);
    close(pout[0]);
    waitpid(pid, &status, 0);
    *olen = o;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
}

/* text punched to user, in class cls, named name ("fn ft"): cut into
   80-column EBCDIC cards */
static int punchto(const char *user, const char *cls, const char *name, const char *text, size_t len)
{
    char p[64], cmd[128], resp[512];
    unsigned char c[80];
    size_t i = 0;
    int fd, k, rc = 0;

    snprintf(cmd, sizeof cmd, "spool %s to %s class %s", pundev + 5, user, cls);
    cp(cmd, resp, sizeof resp);
    if (strstr(resp, "INVALID") || strstr(resp, "NOT")) {
        logf_("cannot spool to %s: %s", user, resp);
        snprintf(cmd, sizeof cmd, "spool %s off", pundev + 5);
        cp(cmd, NULL, 0);
        return -1;
    }
    snprintf(p, sizeof p, "/dev/vmpun-%s", pundev);
    fd = open(p, O_WRONLY);
    if (fd < 0) { logf_("%s: %s", p, strerror(errno)); return -1; }
    while (i < len) {
        size_t e = i;
        while (e < len && text[e] != '\n') e++;
        do {                    /* a line of more than 80: more cards */
            memset(c, 0x40, 80);
            for (k = 0; k < 80 && i < e; k++, i++)
                c[k] = a2e[(unsigned char) (text[i] == '\t' ? ' ' : text[i])];
            if (write(fd, c, 80) != 80) rc = -1;
        } while (i < e);
        i = e + 1;
    }
    close(fd);
    snprintf(cmd, sizeof cmd, "close %s name %s", pundev + 5, name);
    cp(cmd, NULL, 0);
    snprintf(cmd, sizeof cmd, "spool %s off", pundev + 5);
    cp(cmd, NULL, 0);
    return rc;
}

/* 9.17: a file punched as it is (binary), 80 bytes a card, the last
   card padded with X'00'; read in pieces, as it can be larger than the
   machine's storage */
static int punchfile(const char *user, const char *cls, const char *name, const char *path)
{
    char p[64], cmd[128], resp[512];
    static unsigned char buf[80 * 100];
    int in, fd, rc = 0;
    ssize_t n;
    if ((in = open(path, O_RDONLY)) < 0) { logf_("outbox: %s: %s", path, strerror(errno)); return -1; }
    snprintf(cmd, sizeof cmd, "spool %s to %s class %s", pundev + 5, user, cls);
    cp(cmd, resp, sizeof resp);
    if (strstr(resp, "INVALID") || strstr(resp, "NOT")) {
        logf_("cannot spool to %s: %s", user, resp);
        snprintf(cmd, sizeof cmd, "spool %s off", pundev + 5);
        cp(cmd, NULL, 0);
        close(in);
        return -1;
    }
    snprintf(p, sizeof p, "/dev/vmpun-%s", pundev);
    fd = open(p, O_WRONLY);
    if (fd < 0) { logf_("%s: %s", p, strerror(errno)); close(in); return -1; }
    for (;;) {
        size_t got = 0;
        while (got < sizeof buf && (n = read(in, buf + got, sizeof buf - got)) > 0) got += n;
        if (got == 0) break;
        if (got % 80) { memset(buf + got, 0, 80 - got % 80); got += 80 - got % 80; }
        for (size_t o = 0; o < got; o += 80)
            if (write(fd, buf + o, 80) != 80) { rc = -1; break; }
        if (rc || got < sizeof buf) break;
    }
    close(fd);
    close(in);
    snprintf(cmd, sizeof cmd, "close %s name %s", pundev + 5, name);
    cp(cmd, NULL, 0);
    snprintf(cmd, sizeof cmd, "spool %s off", pundev + 5);
    cp(cmd, NULL, 0);
    return rc;
}

static int punch(const char *user, const char *text, size_t len)
{
    return punchto(user, "a", "VMSPOOL REPLY", text, len);
}

/* 9.18: the outbox.  Other programs (vmmaild, the mail service) hand
   vmspoold files to punch, so that one program owns the punch: a file
   <spool>/outq/NAME.out, written elsewhere and renamed into place, whose
   first line is
       TO userid [CLASS c] [NAME fn ft] [FILE path]
   and whose other lines are punched to userid (FILE: that file instead,
   binary, 80 bytes a card).  Done files are removed;
   files that cannot be punched go to <spool>/outq/failed. */
static void outbox(void)
{
    char d[256], f[512], g[512];
    DIR *dir;
    struct dirent *e;
    snprintf(d, sizeof d, "%s/outq", sdir);
    if (!(dir = opendir(d))) return;
    while ((e = readdir(dir))) {
        size_t n = strlen(e->d_name), len;
        char *b, *nl, user[16] = "", cls[4] = "a", fn[16] = "", ft[16] = "", name[40], path[256] = "";
        FILE *fp;
        struct stat st;
        if (n < 5 || strcmp(e->d_name + n - 4, ".out")) continue;
        snprintf(f, sizeof f, "%s/%s", d, e->d_name);
        if (stat(f, &st) || !(fp = fopen(f, "r"))) continue;
        b = malloc(st.st_size + 1);
        len = b ? fread(b, 1, st.st_size, fp) : 0;
        fclose(fp);
        if (!b) continue;
        b[len] = 0;
        nl = strchr(b, '\n');
        if (nl) {
            char *t, *save, *w[16];
            int k = 0;
            *nl = 0;
            for (t = strtok_r(b, " ", &save); t && k < 16; t = strtok_r(NULL, " ", &save)) w[k++] = t;
            if (k >= 2 && !strcasecmp(w[0], "TO")) {
                int j;
                snprintf(user, sizeof user, "%.8s", w[1]);
                for (j = 2; j < k; j++) {
                    if (!strcasecmp(w[j], "CLASS") && j + 1 < k) snprintf(cls, sizeof cls, "%.1s", w[++j]);
                    else if (!strcasecmp(w[j], "FILE") && j + 1 < k) snprintf(path, sizeof path, "%s", w[++j]);
                    else if (!strcasecmp(w[j], "NAME") && j + 2 < k) {
                        snprintf(fn, sizeof fn, "%.8s", w[j + 1]); snprintf(ft, sizeof ft, "%.8s", w[j + 2]); j += 2;
                    }
                }
            }
        }
        if (fn[0]) snprintf(name, sizeof name, "%s %s", fn, ft); else strcpy(name, "VMSPOOL OUTBOX");
        if (user[0] && nl && (path[0] ? punchfile(user, cls, name, path)
                              : punchto(user, cls, name, nl + 1, len - (nl + 1 - b))) == 0) {
            unlink(f);
            logf_("outbox: punched %s to %s", e->d_name, user);
        } else {
            snprintf(g, sizeof g, "%s/failed", d); mkdir(g, 0700);
            snprintf(g, sizeof g, "%s/failed/%s", d, e->d_name);
            rename(f, g);
            logf_("outbox: %s could not be punched%s (kept in failed)", e->d_name, "");
        }
        free(b);
    }
    closedir(dir);
}

static void keep(const unsigned char *b, size_t len)
{
    char p[256], line[81];
    size_t i;
    FILE *f;
    snprintf(p, sizeof p, "%s/unknown", sdir);
    mkdir(sdir, 0755); mkdir(p, 0700);
    snprintf(p, sizeof p, "%s/unknown/%ld-%d.txt", sdir, (long) time(NULL), getpid());
    f = fopen(p, "w");
    if (!f) return;
    for (i = 0; i < len / 80; i++) { card(b, i, line); fprintf(f, "%s\n", line); }
    fclose(f);
    logf_("not a request (%s), kept in %s", "first card", p);
}

static void serve(const unsigned char *b, size_t len)
{
    char c1[81], svc[17], path[256], line[81];
    char *argv[24], *words, *tok, *from = NULL, *out, *in;
    size_t i, ncards = len / 80, inlen = 0, olen = 0;
    int argc = 0, rc;
    struct stat st;

    card(b, 0, c1);
    words = strdup(c1);
    tok = strtok(words, " ");
    if (!tok || strlen(tok) > 16) { keep(b, len); free(words); return; }
    for (i = 0; tok[i]; i++) svc[i] = tolower((unsigned char) tok[i]);
    svc[i] = 0;
    tok = strtok(NULL, " ");
    if (tok && !strcasecmp(tok, "FROM")) from = strtok(NULL, " ");
    if (!valid_service(svc) || !from || !valid_user(from)) {
        keep(b, len); free(words); return;
    }
    for (i = 0; from[i]; i++) from[i] = toupper((unsigned char) from[i]);
    snprintf(path, sizeof path, "%s/%s", hdir, svc);
    argv[argc++] = path;
    while ((tok = strtok(NULL, " ")) && argc < 22) argv[argc++] = tok;
    argv[argc] = NULL;

    in = malloc(ncards * 81 + 1);
    out = malloc(MAXOUT + 64);
    for (i = 1; i < ncards; i++) {
        card(b, i, line);
        inlen += sprintf(in + inlen, "%s\n", line);
    }
    if (stat(path, &st) < 0 || !(st.st_mode & S_IXUSR) || !S_ISREG(st.st_mode)) {
        olen = snprintf(out, MAXOUT, "VMSPOOL RC 404\nvmspoold: no service %s\n", svc);
        rc = 404;
    } else {
        size_t h = snprintf(out, MAXOUT, "VMSPOOL RC    \n");
        /* 9.13b: the input cards as they came (EBCDIC, 80 bytes each)
           in a file named by VMSPOOL_RAW, for handlers that pass
           binary data on (nje PUNCH) */
        char raw[256];
        FILE *rf;
        snprintf(raw, sizeof raw, "%s/raw-%ld-%d.cards", sdir, (long) time(NULL), getpid());
        if ((rf = fopen(raw, "wb")) != NULL) {
            if (len > 80) fwrite(b + 80, 1, len - 80 - (len % 80), rf);
            fclose(rf);
            setenv("VMSPOOL_RAW", raw, 1);
        }
        rc = run(path, argv, from, svc, in, inlen, out + h, &olen);
        unsetenv("VMSPOOL_RAW");
        unlink(raw);
        olen += h;
        memcpy(out + 11, line, sprintf(line, "%-3d", rc));
    }
    logf_("%s from %s", svc, from);
    if (verbose) fprintf(stderr, "  rc %d, %lu bytes in, %lu out\n", rc,
                         (unsigned long) inlen, (unsigned long) olen);
    punch(from, out, olen);
    free(in); free(out); free(words);
}

int main(int argc, char **argv)
{
    int o, i;
    for (i = 0; i < 256; i++) a2e[e2a[i]] = (unsigned char) i;
    while ((o = getopt(argc, argv, "r:p:d:s:i:t:1v")) != -1) {
        switch (o) {
        case 'r': rdrdev = optarg; break;
        case 'p': pundev = optarg; break;
        case 'd': hdir = optarg; break;
        case 's': sdir = optarg; break;
        case 'i': interval = atoi(optarg); break;
        case 't': tmo = atoi(optarg); break;
        case '1': once = 1; break;
        case 'v': verbose = 1; break;
        default:
            fprintf(stderr, "usage: vmspoold [-r rdr] [-p pun] [-d dir] [-s spool] "
                    "[-i secs] [-t secs] [-1] [-v]\n");
            return 2;
        }
    }
    signal(SIGPIPE, SIG_IGN);
    online(rdrdev);
    online(pundev);
    { char c[32]; snprintf(c, sizeof c, "spool %s off", pundev + 5); cp(c, NULL, 0); }
    whoami();
    logf_("started as %s, reader %s", myid[0] ? myid : "?", rdrdev);
    for (;;) {
        size_t len = 0;
        unsigned char *b = waiting() ? readfile(&len) : NULL;
        if (b) {
            if (len >= 80) serve(b, len);
            free(b);
            continue;           /* the next file at once */
        }
        outbox();
        if (once) break;
        sleep(interval);
    }
    return 0;
}
