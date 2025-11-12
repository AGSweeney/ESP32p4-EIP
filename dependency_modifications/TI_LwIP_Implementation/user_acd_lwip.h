/**
 * @file user_acd_lwip.h
 * @brief Include file for ACD functionalities of EIP application for LwIP
 *
 * Adapted from user_acd_ndk.h for LwIP stack integration
 */

#ifndef _USER_ACD_LWIP_H_
#define _USER_ACD_LWIP_H_

#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "protocols/ethernetip_adapter/include/app_api.h"
#include "protocols/ethernetip_adapter/include/acd_api.h"
#include "ti/drv/icss_emac/icss_emacDrv.h"
#include "protocols/ethernetip_adapter/include/user_acd.h"
#include "eip_main.h"

#define ACD_LINKDOWN -10
#define ACD_LED_CONFLICT 1

#define ARPHDR_SIZE 28
#define ETHHDR_SIZE 14

void EIPACD_init(struct netif *netif);
void EIPACD_exit(void);
int EIPACD_start(void);
void EIPACD_stop(void);
int EIPACD_conflictDetected(ST_ACD_CONFLICT_DETECTED stAcdConflictDetected);
int EIPACD_sendArpFrame(unsigned short wFrameSize, unsigned char *byFrame);
int EIPACD_sendArpFrameUnicast(unsigned short wFrameSize, unsigned char *byFrame);
int EIPACD_event(unsigned long dwEvent);
void EIPACD_registerLinkCallBack(ICSSEMAC_Handle pruIcssHandle);
void EIPACD_deregisterLinkCallBack(ICSSEMAC_Handle pruIcssHandle);
int EIPACD_getACDLEDStat(void);
void EIPACD_updateLinkStatus(uint32_t linkStatus, uint32_t portNum);
void EIPACD_linkStatusPort0(uint8_t linkStatus, UArg arg);
void EIPACD_linkStatusPort1(uint8_t linkStatus, UArg arg);

err_t EIPACD_netif_input(struct pbuf *p, struct netif *inp);
err_t EIPACD_netif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr);
int EIPACD_validateIPAddress(ip4_addr_t *ipaddr);

#endif

