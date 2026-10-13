/* config.h -- rabbitmq-c 0.11.0 for CMS (VM/370plus 9.7, GCC 13, cmsrt) */
#define AMQP_POSIX_TIMER_API 1
#define HAVE_SELECT 1
#define AMQ_PLATFORM "VM/370plus CMS"
#define AMQP_STATIC 1
#define AMQP_BUILD 1
#define CLOCK_MONOTONIC 4          /* newlib leaves it out; amqp_cms.c ignores the id */
