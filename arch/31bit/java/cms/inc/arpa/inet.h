/* arpa/inet.h for CMS: big-endian, so the byte-order calls are identity */
#ifndef _CMS_ARPA_INET_H
#define _CMS_ARPA_INET_H
#include <stdint.h>
#define htonl(x) ((uint32_t)(x))
#define ntohl(x) ((uint32_t)(x))
#define htons(x) ((uint16_t)(x))
#define ntohs(x) ((uint16_t)(x))
#endif
