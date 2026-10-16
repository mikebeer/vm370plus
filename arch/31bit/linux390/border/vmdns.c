/* vmdns -- VM/370plus 9.40: the guest LAN's name server, on BORDER.
 *
 *   vmdns [-c /etc/vmdns.conf] [-f]        (-f: stay in the foreground)
 *
 * Answers for the local zone itself (authoritative) and passes every
 * other question on to the upstream name server (by default the PC),
 * relaying the answer back.  UDP only.  The configuration:
 *
 *   domain  vm370plus.lan       the zone (default vm370plus.lan)
 *   listen  10.3.0.1            address to serve on (default all)
 *   upstream 10.1.4.1           where other names go (no upstream: refused)
 *   host    maint 10.3.0.2      an A record (and its PTR); repeatable
 *   mx      10 mail             an MX record for the domain; repeatable
 *   ttl     300                 for local answers (default 300)
 *
 * In the zone: NAME.DOMAIN or a bare NAME (one label) -- A, PTR (for the
 * in-addr.arpa of a host's address), MX, NS and SOA of the domain, ANY.
 * A name in the zone that is not a host gets NXDOMAIN; a host asked for
 * another type (AAAA, ...) gets an empty answer.  Reverse questions for
 * an address of no host are passed upstream.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXH 256
#define MAXMX 8
#define PEND 256

static char domain[256] = "vm370plus.lan";
static struct { char name[64]; struct in_addr a; } host[MAXH];
static int nhost;
static struct { int pref; char name[256]; } mx[MAXMX];
static int nmx;
static unsigned ttl = 300;
static struct in_addr listen_a, up_a;
static int have_up;

static struct { int used; unsigned short cid, uid; struct sockaddr_in from; time_t t; } pend[PEND];
static unsigned short next_id;

static void lower(char *s) { for (; *s; s++) *s = tolower((unsigned char)*s); }

static void readconf(const char *fn)
{
    FILE *f = fopen(fn, "r");
    char l[512], k[64], v1[256], v2[256];
    if (!f) { perror(fn); exit(1); }
    while (fgets(l, sizeof l, f)) {
        int n;
        char *h = strchr(l, '#');
        if (h) *h = 0;
        n = sscanf(l, "%63s %255s %255s", k, v1, v2);
        if (n < 2) continue;
        lower(k);
        if (!strcmp(k, "domain")) { snprintf(domain, sizeof domain, "%s", v1); lower(domain); }
        else if (!strcmp(k, "listen")) inet_aton(v1, &listen_a);
        else if (!strcmp(k, "upstream")) have_up = inet_aton(v1, &up_a);
        else if (!strcmp(k, "ttl")) ttl = atoi(v1);
        else if (!strcmp(k, "host") && n == 3 && nhost < MAXH) {
            snprintf(host[nhost].name, sizeof host[0].name, "%s", v1);
            lower(host[nhost].name);
            if (inet_aton(v2, &host[nhost].a)) nhost++;
        } else if (!strcmp(k, "mx") && n == 3 && nmx < MAXMX) {
            mx[nmx].pref = atoi(v1);
            snprintf(mx[nmx].name, sizeof mx[0].name, "%s", v2);
            lower(mx[nmx].name);
            nmx++;
        } else fprintf(stderr, "vmdns: %s: ignored: %s %s\n", fn, k, v1);
    }
    fclose(f);
}

/* the question name at q (in packet p of n bytes), as text; returns the
 * offset after it, or -1 */
static int qname(const unsigned char *p, int n, int o, char *out, int max)
{
    int len = 0;
    for (;;) {
        int c;
        if (o >= n) return -1;
        c = p[o++];
        if (c == 0) break;
        if (c & 0xC0) return -1;            /* no pointers in a question */
        if (o + c > n || len + c + 2 > max) return -1;
        if (len) out[len++] = '.';
        memcpy(out + len, p + o, c);
        len += c; o += c;
    }
    out[len] = 0;
    lower(out);
    return o;
}

/* a name written as labels */
static int putname(unsigned char *b, int o, int max, const char *name)
{
    const char *s = name;
    while (*s) {
        const char *d = strchr(s, '.');
        int l = d ? (int)(d - s) : (int)strlen(s);
        if (l == 0 || l > 63 || o + l + 2 > max) return -1;
        b[o++] = l; memcpy(b + o, s, l); o += l;
        s += l; if (*s) s++;
    }
    if (o + 1 > max) return -1;
    b[o++] = 0;
    return o;
}

static int put16(unsigned char *b, int o, unsigned v) { b[o] = v >> 8; b[o + 1] = v; return o + 2; }
static int put32(unsigned char *b, int o, unsigned v) { o = put16(b, o, v >> 16); return put16(b, o, v & 0xFFFF); }

/* the host of a name in the zone ("" for the domain itself), or NULL */
static const char *inzone(const char *name)
{
    size_t ln = strlen(name), ld = strlen(domain);
    if (!strcmp(name, domain)) return "";
    if (ln > ld + 1 && name[ln - ld - 1] == '.' && !strcmp(name + ln - ld, domain)) {
        static char h[256];
        memcpy(h, name, ln - ld - 1); h[ln - ld - 1] = 0;
        return strchr(h, '.') ? NULL : h;    /* one label below the domain */
    }
    if (ln && !strchr(name, '.')) return name;   /* a bare name */
    return NULL;
}

static int findhost(const char *h)
{
    int i;
    for (i = 0; i < nhost; i++) if (!strcmp(host[i].name, h)) return i;
    return -1;
}

/* a PTR question for an address of a host: its index, else -1 */
static int findptr(const char *name)
{
    unsigned a, b, c, d;
    char rest[64];
    int i;
    if (sscanf(name, "%u.%u.%u.%u.%63s", &d, &c, &b, &a, rest) != 5 || strcmp(rest, "in-addr.arpa")) return -1;
    for (i = 0; i < nhost; i++)
        if (ntohl(host[i].a.s_addr) == (a << 24 | b << 16 | c << 8 | d)) return i;
    return -1;
}

/* one resource record, owner a pointer to the question (offset 12) or a name */
static int rr(unsigned char *b, int o, int max, const char *owner, unsigned type)
{
    if (owner) { if ((o = putname(b, o, max, owner)) < 0) return -1; }
    else { if (o + 2 > max) return -1; o = put16(b, o, 0xC00C); }
    if (o + 10 > max) return -1;
    o = put16(b, o, type); o = put16(b, o, 1); o = put32(b, o, ttl);
    return o;                                /* the caller writes RDLENGTH, RDATA */
}

static int rr_a(unsigned char *b, int o, int max, const char *owner, struct in_addr a)
{
    if ((o = rr(b, o, max, owner, 1)) < 0 || o + 6 > max) return -1;
    o = put16(b, o, 4); memcpy(b + o, &a, 4);
    return o + 4;
}

static int rr_name(unsigned char *b, int o, int max, unsigned type, int pref, const char *target)
{
    int lo, e;
    if ((o = rr(b, o, max, NULL, type)) < 0 || o + 4 > max) return -1;
    lo = o; o += 2;
    if (type == 15) o = put16(b, o, pref);
    if ((e = putname(b, o, max, target)) < 0) return -1;
    put16(b, lo, e - lo - 2);
    return e;
}

static int rr_soa(unsigned char *b, int o, int max)
{
    char ns[300], hm[300];
    int lo;
    snprintf(ns, sizeof ns, "border.%s", domain);
    snprintf(hm, sizeof hm, "hostmaster.%s", domain);
    if ((o = rr(b, o, max, NULL, 6)) < 0) return -1;
    lo = o; o += 2;
    if ((o = putname(b, o, max, ns)) < 0 || (o = putname(b, o, max, hm)) < 0 || o + 20 > max) return -1;
    o = put32(b, o, 1); o = put32(b, o, 3600); o = put32(b, o, 600);
    o = put32(b, o, 86400); o = put32(b, o, ttl);
    put16(b, lo, o - lo - 2);
    return o;
}

static const char *fqdn(const char *h)
{
    static char f[320];
    if (strchr(h, '.')) return h;
    snprintf(f, sizeof f, "%s.%s", h, domain);
    return f;
}

/* the answer to a question in the zone, in r; its length, or 0 when the
 * question is not ours (pass it on) */
static int local(const unsigned char *q, int qn, int qend, const char *name, unsigned qt, unsigned char *r, int max)
{
    const char *h = inzone(name);
    int i, o, an = 0, ar = 0, rcode = 0, hi = -1;
    if (qt == 12) {                          /* PTR */
        if ((hi = findptr(name)) < 0) return 0;
    } else if (!h) return 0;
    if (qend > max) return 0;
    memcpy(r, q, qend);                      /* header and question */
    o = qend;
    if (qt == 12) {
        o = rr_name(r, o, max, 12, 0, fqdn(host[hi].name)); an = 1;
    } else if (*h == 0) {                    /* the domain itself */
        if (qt == 15 || qt == 255)
            for (i = 0; i < nmx && o > 0; i++) { o = rr_name(r, o, max, 15, mx[i].pref, fqdn(mx[i].name)); an++; }
        if ((qt == 2 || qt == 255) && o > 0) { char ns[300]; snprintf(ns, sizeof ns, "border.%s", domain); o = rr_name(r, o, max, 2, 0, ns); an++; }
        if ((qt == 6 || qt == 255) && o > 0) { o = rr_soa(r, o, max); an++; }
        /* the exchangers' addresses, as additional records */
        if (qt == 15 || qt == 255)
            for (i = 0; i < nmx && o > 0; i++) {
                const char *x = inzone(mx[i].name) ? inzone(mx[i].name) : "";
                int k = *x ? findhost(x) : -1;
                if (k >= 0) { o = rr_a(r, o, max, fqdn(mx[i].name), host[k].a); ar++; }
            }
    } else if ((hi = findhost(h)) >= 0) {
        if (qt == 1 || qt == 255) { o = rr_a(r, o, max, NULL, host[hi].a); an = 1; }
    } else rcode = 3;                        /* NXDOMAIN */
    if (o < 0) return 0;
    r[2] = 0x84 | (q[2] & 0x01);             /* QR, AA, RD as asked */
    r[3] = 0x80 | rcode;                     /* RA */
    put16(r, 4, 1); put16(r, 6, an); put16(r, 8, 0); put16(r, 10, ar);
    return o;
}

static int answer_refused(const unsigned char *q, int qend, unsigned char *r)
{
    memcpy(r, q, qend);
    r[2] = 0x80 | (q[2] & 0x01); r[3] = 0x80 | 5;
    put16(r, 4, 1); put16(r, 6, 0); put16(r, 8, 0); put16(r, 10, 0);
    return qend;
}

int main(int argc, char **argv)
{
    const char *conf = "/etc/vmdns.conf";
    int fg = 0, i, s, u;
    struct sockaddr_in sa;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-c") && i + 1 < argc) conf = argv[++i];
        else if (!strcmp(argv[i], "-f")) fg = 1;
        else { fprintf(stderr, "usage: vmdns [-c conf] [-f]\n"); return 4; }
    }
    readconf(conf);
    if (findhost("border") < 0 && nhost < MAXH && listen_a.s_addr) {
        strcpy(host[nhost].name, "border"); host[nhost].a = listen_a; nhost++;
    }
    s = socket(AF_INET, SOCK_DGRAM, 0);
    u = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0 || u < 0) { perror("socket"); return 1; }
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(getenv("VMDNS_PORT") ? atoi(getenv("VMDNS_PORT")) : 53);
    sa.sin_addr = listen_a;
    if (bind(s, (struct sockaddr *)&sa, sizeof sa) < 0) { perror("vmdns: bind"); return 1; }
    signal(SIGPIPE, SIG_IGN);
    fprintf(stderr, "vmdns: zone %s, %d host(s), %d MX, upstream %s\n", domain, nhost, nmx,
            have_up ? inet_ntoa(up_a) : "none");
    if (!fg && daemon(0, 0) < 0) { perror("daemon"); return 1; }
    srand(time(NULL) ^ getpid());
    next_id = rand();
    for (;;) {
        fd_set rf;
        unsigned char q[1500], r[1500];
        struct sockaddr_in from;
        socklen_t fl = sizeof from;
        int n, m = s > u ? s : u;
        FD_ZERO(&rf); FD_SET(s, &rf); FD_SET(u, &rf);
        if (select(m + 1, &rf, NULL, NULL, NULL) < 0) { if (errno == EINTR) continue; perror("select"); return 1; }
        if (FD_ISSET(s, &rf) && (n = recvfrom(s, q, sizeof q, 0, (struct sockaddr *)&from, &fl)) >= 12) {
            char name[256];
            int qe, rl;
            unsigned qt;
            if ((q[2] & 0x80) || q[4] != 0 || q[5] != 1) continue;   /* a query with one question */
            if ((qe = qname(q, n, 12, name, sizeof name)) < 0 || qe + 4 > n) continue;
            qt = q[qe] << 8 | q[qe + 1];
            qe += 4;
            if ((rl = local(q, n, qe, name, qt, r, sizeof r)) > 0) {
                sendto(s, r, rl, 0, (struct sockaddr *)&from, fl);
            } else if (have_up) {
                int k;
                struct sockaddr_in to;
                time_t now = time(NULL);
                for (k = 0; k < PEND; k++) if (!pend[k].used || now - pend[k].t > 10) break;
                if (k == PEND) k = next_id % PEND;   /* all busy: the oldest slot goes */
                pend[k].used = 1; pend[k].cid = q[0] << 8 | q[1]; pend[k].from = from; pend[k].t = now;
                pend[k].uid = (unsigned short)((next_id++ & 0xFF00) | k);
                q[0] = pend[k].uid >> 8; q[1] = pend[k].uid;
                memset(&to, 0, sizeof to);
                to.sin_family = AF_INET; to.sin_port = htons(53); to.sin_addr = up_a;
                sendto(u, q, n, 0, (struct sockaddr *)&to, sizeof to);
            } else {
                rl = answer_refused(q, qe, r);
                sendto(s, r, rl, 0, (struct sockaddr *)&from, fl);
            }
        }
        if (FD_ISSET(u, &rf) && (n = recvfrom(u, r, sizeof r, 0, (struct sockaddr *)&from, &fl)) >= 12) {
            unsigned id = r[0] << 8 | r[1];
            int k = id & 0xFF;
            if (from.sin_addr.s_addr != up_a.s_addr || !pend[k].used || pend[k].uid != id) continue;
            r[0] = pend[k].cid >> 8; r[1] = pend[k].cid;
            sendto(s, r, n, 0, (struct sockaddr *)&pend[k].from, sizeof pend[k].from);
            pend[k].used = 0;
        }
    }
}
