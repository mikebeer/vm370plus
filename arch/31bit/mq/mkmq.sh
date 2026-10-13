#!/bin/sh
# mkmq.sh -- build MQAMQP TEXT (9.7): rabbitmq-c 0.11 on NICOF's sockets,
# GCC 13 (-m31, cmsrt).  On CMS (IPL 290): LOAD MQAMQP (CLEAR, GENMOD.
set -e
H=$(cd "$(dirname "$0")" && pwd); N=$H/../nicof/gcc13; A=$H/rabbitmq-c
CFLAGS_EXTRA="-D__CMSRT__ -DHAVE_CONFIG_H -I$A -I$N -w" sh $H/../m5f/crt.sh ${1:-$H/MQAMQP.text} \
  $H/cms/mqamqp.c $A/amqp_api.c $A/amqp_connection.c $A/amqp_consumer.c \
  $A/amqp_framing.c $A/amqp_mem.c $A/amqp_socket.c $A/amqp_table.c \
  $A/amqp_tcp_socket.c $A/amqp_time.c $A/amqp_url.c $A/amqp_cms.c \
  $N/ncfsockt.c $N/nicofclt.c $N/ncfglue.c $N/ncfintr.s
