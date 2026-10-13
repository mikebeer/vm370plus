# MQ for CMS (9.7): message queues on RabbitMQ

`MQ EXEC` and `MQAMQP MODULE` give CMS programs message queues on a
RabbitMQ broker (AMQP 0-9-1). The module is the C client library
**rabbitmq-c 0.11.0** (MIT, `rabbitmq-c/LICENSE`) built with GCC 13 (-m31,
cmsrt) on NICOF's sockets (`../nicof/gcc13`), so it runs under the EC-mode
CMS (`IPL 290`) and reaches the broker through the PC: the same broker the
cREXX package mikebeer/mq talks to.

    MQ PUT queue message      put a persistent message (the queue is declared
                              durable when it does not exist)
    MQ GET queue              get one message and type it; rc 4: queue empty
    MQ GET queue (STACK       ... onto the program stack (for EXECs)
    MQ DEPTH queue            the number of messages waiting
    MQ PURGE queue            drop them all

The broker is named in `MQ OPTIONS` (any accessed disk), one `key value` per
line: `HOST`, `PORT`, `USER`, `PASSWORD`, `VHOST`; the defaults are
`localhost 5672 guest guest /`. Host and port are seen from the PC (NICOF's
proxy makes the connection), and RabbitMQ's `guest` user may log in from
localhost only. Message bodies are UTF-8 on the broker and Latin-1 on CMS.

`MQTEST EXEC` puts three messages, shows the depth and takes them back
through the stack.

## Build

    sh mkmq.sh                 # -> MQAMQP.text
    LOAD MQAMQP (CLEAR         # on CMS, IPL 290
    GENMOD MQAMQP

## The port (rabbitmq-c, `#ifdef __CMSRT__`)

- `amqp_socket.c`: the socket comes from NICOF (`gethostbyname`, `socket`,
  blocking `connect`); `amqp_poll` waits in NICOF's `select()` for reads
  and lets writes go ahead.
- `amqp_tcp_socket.c`: at most 2,000 bytes per `send`/`recv` (NICOF moves 2
  KB a call); a `recv` asks `select()` first, so the library can wait with
  its own deadline (NICOF's `recv` blocks).
- `amqp_time.c`, `amqp_cms.c`: a monotonic clock from `gettimeofday`.
- `amqp_private.h`: no network headers.

## Verified (14 Oct, run mq1; RabbitMQ 3.12 on the PC)

PUT from CMS and `amqp-get` on the PC; `amqp-publish` on the PC and GET on
CMS; DEPTH; GET of an empty queue (rc 4); MQTEST (3 messages through the
stack); a GET whose file cannot be written leaves the message on the queue.
