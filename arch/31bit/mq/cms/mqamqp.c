/*
** MQAMQP -- VM/370plus 9.7: message queues for CMS on a RabbitMQ broker
** (AMQP 0-9-1), through rabbitmq-c 0.11 and NICOF's sockets.  GCC 13,
** cmsrt; IPL 290.  Usually called by MQ EXEC:
**
**   MQAMQP PUT queue message     put a persistent message (the queue is
**                                declared durable if it does not exist)
**   MQAMQP GET queue [fileid]    get one message and type it (or write it,
**                                one record per line, to fileid); rc 4: empty
** Message bodies are UTF-8 on the broker, Latin-1 here (CMS 1047).
**   MQAMQP DEPTH queue           the number of messages waiting
**   MQAMQP PURGE queue           drop every message
**
** The broker comes from MQ OPTIONS (any disk), lines "key value":
**   HOST localhost   PORT 5672   USER guest   PASSWORD guest   VHOST /
** Host and port are as the PC sees them (NICOF's proxy connects).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "amqp.h"
#include "amqp_tcp_socket.h"

static char host[128] = "localhost", user[64] = "guest", pw[64] = "guest",
            vhost[64] = "/";
static int port = 5672;

static void options(void)
{
  char line[200], k[32], v[160];
  FILE *f = fopen("MQ OPTIONS", "r");
  if (!f) return;
  while (fgets(line, sizeof line, f)) {
    if (sscanf(line, "%31s %159s", k, v) != 2 || k[0] == '*') continue;
    for (char *p = k; *p; p++) *p = toupper((unsigned char)*p);
    if (!strcmp(k, "HOST")) strcpy(host, v);
    else if (!strcmp(k, "PORT")) port = atoi(v);
    else if (!strcmp(k, "USER")) strcpy(user, v);
    else if (!strcmp(k, "PASSWORD")) strcpy(pw, v);
    else if (!strcmp(k, "VHOST")) strcpy(vhost, v);
  }
  fclose(f);
}

/* Latin-1 -> UTF-8 (the broker side) */
static int to_utf8(const char *in, char *out, int max)
{
  int n = 0;
  for (; *in && n < max - 2; in++) {
    unsigned char c = (unsigned char)*in;
    if (c < 0x80) out[n++] = c;
    else { out[n++] = 0xC0 | (c >> 6); out[n++] = 0x80 | (c & 0x3F); }
  }
  out[n] = 0;
  return n;
}

/* UTF-8 -> Latin-1 (characters beyond Latin-1 become '?') */
static int from_utf8(const unsigned char *in, int len, char *out)
{
  int n = 0, i = 0;
  while (i < len) {
    unsigned char c = in[i];
    if (c < 0x80) { out[n++] = c; i++; }
    else if ((c & 0xE0) == 0xC0 && i + 1 < len) {
      unsigned v = ((c & 0x1F) << 6) | (in[i + 1] & 0x3F);
      out[n++] = v < 256 ? (char)v : '?'; i += 2;
    } else {
      int k = (c & 0xF0) == 0xE0 ? 3 : (c & 0xF8) == 0xF0 ? 4 : 1;
      out[n++] = '?'; i += k;
    }
  }
  out[n] = 0;
  return n;
}

static int fail(amqp_rpc_reply_t r, const char *what)
{
  if (r.reply_type == AMQP_RESPONSE_NORMAL) return 0;
  if (r.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION)
    printf("MQ: %s: %s\n", what, amqp_error_string2(r.library_error));
  else if (r.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION &&
           r.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
    amqp_channel_close_t *c = (amqp_channel_close_t *)r.reply.decoded;
    printf("MQ: %s: %.*s\n", what, (int)c->reply_text.len,
           (char *)c->reply_text.bytes);
  } else
    printf("MQ: %s failed\n", what);
  return 1;
}

int main(int argc, char *argv[])
{
  amqp_connection_state_t c;
  amqp_socket_t *s;
  char verb[16];
  const char *q;
  int i, rc = 0;
  if (argc < 3) {
    printf("MQ: usage: MQ PUT|GET|DEPTH|PURGE queue [message]\n");
    return 24;
  }
  for (i = 0; argv[1][i] && i < 15; i++) verb[i] = toupper((unsigned char)argv[1][i]);
  verb[i] = 0;
  q = argv[2];
  options();
  c = amqp_new_connection();
  s = amqp_tcp_socket_new(c);
  if (!s || amqp_socket_open(s, host, port)) {
    printf("MQ: cannot reach the broker at %s:%d (NICOF proxies running?)\n", host, port);
    return 28;
  }
  if (fail(amqp_login(c, vhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user, pw), "login"))
    return 32;
  amqp_channel_open(c, 1);
  if (fail(amqp_get_rpc_reply(c), "channel")) return 32;

  if (!strcmp(verb, "PUT")) {
    char msg[4096] = "";
    amqp_basic_properties_t p;
    for (i = 3; i < argc; i++) {
      if (i > 3) strncat(msg, " ", sizeof msg - strlen(msg) - 1);
      strncat(msg, argv[i], sizeof msg - strlen(msg) - 1);
    }
    amqp_queue_declare(c, 1, amqp_cstring_bytes(q), 0, 1, 0, 0, amqp_empty_table);
    if (fail(amqp_get_rpc_reply(c), "declare")) return 36;
    p._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    p.content_type = amqp_cstring_bytes("text/plain");
    p.delivery_mode = 2;                       /* persistent */
    static char u8[8200];
    to_utf8(msg, u8, sizeof u8);
    if (amqp_basic_publish(c, 1, amqp_cstring_bytes(""), amqp_cstring_bytes(q),
                           0, 0, &p, amqp_cstring_bytes(u8)) < 0) {
      printf("MQ: put failed\n"); rc = 40;
    }
  } else if (!strcmp(verb, "GET")) {
    amqp_rpc_reply_t r = amqp_basic_get(c, 1, amqp_cstring_bytes(q), 0);
    if (fail(r, "get")) rc = 36;
    else if (r.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) rc = 4;
    else {
      amqp_message_t m;
      amqp_basic_get_ok_t *ok = (amqp_basic_get_ok_t *)r.reply.decoded;
      if (fail(amqp_read_message(c, 1, &m, 0), "read")) rc = 36;
      else {
        char *t = malloc(m.body.len + 1);
        from_utf8((unsigned char *)m.body.bytes, (int)m.body.len, t);
        if (argc > 3 && argv[3][0]) {          /* to a file */
          FILE *f = fopen(argv[3], "w");
          if (!f) { printf("MQ: cannot write %s\n", argv[3]); rc = 44; }
          else { fprintf(f, "%s\n", t); fclose(f); }
        } else
          printf("%s\n", t);
        free(t);
        /* acknowledged only when delivered: otherwise the broker keeps it */
        if (rc == 0) amqp_basic_ack(c, 1, ok->delivery_tag, 0);
        amqp_destroy_message(&m);
      }
    }
  } else if (!strcmp(verb, "DEPTH")) {
    amqp_queue_declare_ok_t *d =
      amqp_queue_declare(c, 1, amqp_cstring_bytes(q), 1, 0, 0, 0, amqp_empty_table);
    if (fail(amqp_get_rpc_reply(c), "depth")) rc = 36;
    else printf("%u\n", d->message_count);
  } else if (!strcmp(verb, "PURGE")) {
    amqp_queue_purge_ok_t *pk = amqp_queue_purge(c, 1, amqp_cstring_bytes(q));
    if (fail(amqp_get_rpc_reply(c), "purge")) rc = 36;
    else printf("MQ: %u messages purged\n", pk->message_count);
  } else {
    printf("MQ: unknown verb %s\n", verb); rc = 24;
  }
  amqp_channel_close(c, 1, AMQP_REPLY_SUCCESS);
  amqp_connection_close(c, AMQP_REPLY_SUCCESS);
  amqp_destroy_connection(c);
  return rc;
}
