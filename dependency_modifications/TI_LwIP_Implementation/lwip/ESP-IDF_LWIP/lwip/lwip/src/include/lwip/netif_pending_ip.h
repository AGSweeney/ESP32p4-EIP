/**
 * @file
 * Pending IP configuration structure for RFC 5227 compliant static IP assignment
 */

#ifndef LWIP_HDR_NETIF_PENDING_IP_H
#define LWIP_HDR_NETIF_PENDING_IP_H

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC

#include "lwip/ip4_addr.h"
/* Now that netif.h only forward declares this struct, we can include acd.h */
/* acd.h includes netif.h, but netif.h no longer includes netif_pending_ip.h */
#include "lwip/acd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct netif_pending_ip_config {
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  struct acd acd;
  acd_conflict_callback_t user_callback;
  u8_t pending;  /* 1 if waiting for ACD, 0 if assigned */
};

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV4 && LWIP_ACD && LWIP_ACD_RFC5227_COMPLIANT_STATIC */

#endif /* LWIP_HDR_NETIF_PENDING_IP_H */

