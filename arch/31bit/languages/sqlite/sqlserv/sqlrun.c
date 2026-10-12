/* sqlrun.c -- SQLSERV's worker (VM/370plus 9.4): runs the SQL of one
 * request file against one SQLite database and writes everything --
 * rows, counts and errors -- to the response file, so that SQLSERV can
 * punch it back to whoever asked.
 *   SQLRUN db.db request.ft response.ft
 * Output: for each statement with columns a header line (names joined by
 * '|') and one line per row (NULL as an empty field); for a statement
 * that changes rows "(n rows)"; an error as "Error: message" (and the
 * next statement still runs).  Last line: "SQLRUN: n statements, n errors". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

static char *slurp(const char *name)
{
    FILE *f = fopen(name, "r");
    if (!f) return 0;
    size_t cap = 4096, n = 0;
    char *b = malloc(cap);
    int c;
    while (b && (c = fgetc(f)) != EOF) {
        if (n + 2 > cap) b = realloc(b, cap *= 2);
        if (b) b[n++] = (char)c;
    }
    fclose(f);
    if (b) b[n] = 0;
    return b;
}

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: SQLRUN db request response\n"); return 24; }
    char *sql = slurp(argv[2]);
    FILE *out = fopen(argv[3], "w");
    if (!sql || !out) { fprintf(stderr, "SQLRUN: cannot open %s\n", sql ? argv[3] : argv[2]); return 28; }
    sqlite3 *db;
    if (sqlite3_open(argv[1], &db) != SQLITE_OK) {
        fprintf(out, "Error: cannot open database %s: %s\n", argv[1], sqlite3_errmsg(db));
        fclose(out);
        return 8;
    }
    int nstmt = 0, nerr = 0;
    const char *p = sql;
    while (*p) {
        sqlite3_stmt *st = 0;
        const char *tail = 0;
        if (sqlite3_prepare_v2(db, p, -1, &st, &tail) != SQLITE_OK) {
            fprintf(out, "Error: %s\n", sqlite3_errmsg(db));
            nerr++;
            /* skip to the next ';' and go on */
            const char *q = strchr(p, ';');
            if (!q) break;
            p = q + 1;
            continue;
        }
        p = tail;
        if (!st) continue;                      /* white space or comment */
        nstmt++;
        int nc = sqlite3_column_count(st), rows = 0, rc;
        while ((rc = sqlite3_step(st)) == SQLITE_ROW) {
            if (rows++ == 0)
                for (int i = 0; i < nc; i++)
                    fprintf(out, "%s%s", i ? "|" : "", sqlite3_column_name(st, i));
            if (rows == 1) fputc('\n', out);
            for (int i = 0; i < nc; i++) {
                const unsigned char *v = sqlite3_column_text(st, i);
                fprintf(out, "%s%s", i ? "|" : "", v ? (const char *)v : "");
            }
            fputc('\n', out);
        }
        if (rc != SQLITE_DONE) { fprintf(out, "Error: %s\n", sqlite3_errmsg(db)); nerr++; }
        else if (nc == 0 && sqlite3_changes(db) > 0 && !sqlite3_stmt_readonly(st))
            fprintf(out, "(%d rows)\n", sqlite3_changes(db));
        else if (nc > 0 && rows == 0)
            fprintf(out, "(no rows)\n");
        sqlite3_finalize(st);
    }
    fprintf(out, "SQLRUN: %d statements, %d errors\n", nstmt, nerr);
    sqlite3_close(db);
    fclose(out);
    return nerr ? 4 : 0;
}
