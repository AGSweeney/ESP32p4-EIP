/**
 * @file user_acd_lwip.c
 * @brief Contains Functions which implement ACD mechanism for LwIP.
 *
 * Adapted from user_acd_ndk.c for LwIP stack integration
 */

#include <string.h>
#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/ethernet.h"
#include "lwip/prot/ethernet.h"
#include "lwip/sys.h"
#include "lwip/timers.h"
#include "lwip/ip4_addr.h"
#include "lwip/def.h"
#include "protocols/ethernetip_adapter/include/icss_dlr.h"
#include "third_party/protocols/ptp/ptpd/dep/icss_ptpd_driver.h"
#include "ti/drv/icss_emac/icss_emacFwInit.h"
#include "ti/drv/icss_emac/icss_emacCommon.h"
#include "examples/ethernetip_adapter/eip_main.h"
#include "examples/ethernetip_adapter/eip_soc.h"
#include "user_acd_lwip.h"

#define BUFSIZE (ARPHDR_SIZE + ETHHDR_SIZE)
#define ARPBUFSIZE (BUFSIZE * 32)
#define ACDPERIOD 10

#define PORT1_MSG_LINKUP    1
#define PORT1_MSG_LINKDOWN  2
#define MSG_ARP             3
#define MSG_NETSTOP         4
#define PORT2_MSG_LINKUP    5
#define PORT2_MSG_LINKDOWN  6
#define PORTS_MSG_LINKDOWN  7

#define USE_ACD_ARP 1
#define USE_LWIP_ARP 0

#define NUMMSGS       16

typedef struct ACD_Object
{
    sys_mbox_t mbox;
    sys_thread_t thread;
    sys_sem_t validSem;
    sys_sem_t clockSem;
} ACD_Object;

unsigned int acdEventMissed = 0;
static ACD_Object acdObj;
static struct pbuf *arpq_head = NULL;
static struct pbuf *arpq_tail = NULL;
static int validAddress;
static unsigned char macAddr[6];
unsigned int ledACD = 0;
unsigned int acdStart = 0;
unsigned int acdCablePull = 0;
unsigned int acdIPAssign = 0;
unsigned int acdARPFlag = USE_ACD_ARP;
unsigned int linkDownStat = 0;
uint8_t eipLinkStat[2] = {0};

extern unsigned int countACD;
extern TimeSync_ParamsHandle_t timeSyncHandle;
extern EIP_Handle eipHandle;
extern int startupMode;
extern int qcMode;
extern unsigned int eipAppStat;
extern int acdEIPStatus;

struct netif *acd_netif = NULL;

static void acd_timer_callback(void *arg);

void EIPACD_updateLinkStatus(uint32_t linkStatus, uint32_t portNum)
{
    uint8_t validMessage = 1;
    uint32_t msg;

    if(linkStatus)
    {
        if(eipLinkStat[portNum])
        {
            validMessage = 0;
        }
        else
        {
            msg = (portNum * 4) + 1;
        }

        eipLinkStat[portNum] = 1;
        linkDownStat = 1;
    }
    else
    {
        eipLinkStat[portNum] = 0;

        if(eipLinkStat[0] | eipLinkStat[1])
        {
            msg = (portNum * 4) + 2;
            linkDownStat = 1;
        }
        else
        {
            if(!linkDownStat)
            {
                validMessage = 0;
            }

            msg = PORTS_MSG_LINKDOWN;
            linkDownStat = 0;
        }
    }

    if(validMessage && acdObj.mbox != NULL)
    {
        sys_mbox_trypost(acdObj.mbox, (void *)(uintptr_t)msg);
    }
}

void EIPACD_linkStatusPort0(uint8_t linkStatus, UArg arg)
{
    EIP_DLR_TIMESYNC_port0ProcessLinkBrk(linkStatus, eipHandle);
    EIPACD_updateLinkStatus(linkStatus, 0);
}

void EIPACD_linkStatusPort1(uint8_t linkStatus, UArg arg)
{
    EIP_DLR_TIMESYNC_port1ProcessLinkBrk(linkStatus, eipHandle);
    EIPACD_updateLinkStatus(linkStatus, 1);
}

static err_t EIPACD_arp_input(struct pbuf *p, struct netif *netif)
{
    if(acdStart && p != NULL)
    {
        struct pbuf *arp_copy = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
        if(arp_copy != NULL)
        {
            pbuf_copy(arp_copy, p);
            
            SYS_ARCH_DECL_PROTECT(lev);
            SYS_ARCH_PROTECT(lev);
            if(arpq_tail == NULL)
            {
                arpq_head = arpq_tail = arp_copy;
            }
            else
            {
                arpq_tail->next = arp_copy;
                arpq_tail = arp_copy;
            }
            SYS_ARCH_UNPROTECT(lev);

            if(acdObj.mbox != NULL)
            {
                uint32_t msg = MSG_ARP;
                sys_mbox_trypost(acdObj.mbox, (void *)(uintptr_t)msg);
            }
        }
    }
    
    return ERR_OK;
}

int EIPACD_validateIPAddress(ip4_addr_t *ipaddr)
{
    if(acd_netif == NULL)
    {
        return 0;
    }

    memcpy(macAddr, acd_netif->hwaddr, 6);

    ACD_Start(htonl(ip4_addr_get_u32(ipaddr)), macAddr, qcMode);
    EIPACD_start();

    sys_sem_wait(acdObj.validSem);

    return validAddress;
}

static void EIPACD_sendLwIPArpPacket(unsigned char *arp, uint32_t IPDst)
{
    struct pbuf *p;
    struct eth_hdr *ethhdr;
    struct etharp_hdr *arp_hdr;

    if(acd_netif == NULL)
    {
        return;
    }

    p = pbuf_alloc(PBUF_LINK, BUFSIZE, PBUF_RAM);
    if(p == NULL)
    {
        return;
    }

    ethhdr = (struct eth_hdr *)p->payload;
    arp_hdr = (struct etharp_hdr *)((unsigned char *)ethhdr + SIZEOF_ETH_HDR);

    memcpy(ethhdr->dest.addr, arp, 6);
    memcpy(ethhdr->src.addr, acd_netif->hwaddr, 6);
    ethhdr->type = PP_HTONS(ETHTYPE_ARP);

    memcpy(arp_hdr, arp + ETHHDR_SIZE, ARPHDR_SIZE);

    acd_netif->linkoutput(acd_netif, p);
    pbuf_free(p);
}

static void EIPACD_sendLwIPArpPacketUnicast(unsigned char *arp, uint32_t IPDst)
{
    struct pbuf *p;
    struct eth_hdr *ethhdr;
    struct etharp_hdr *arp_hdr;

    if(acd_netif == NULL)
    {
        return;
    }

    p = pbuf_alloc(PBUF_LINK, BUFSIZE, PBUF_RAM);
    if(p == NULL)
    {
        return;
    }

    ethhdr = (struct eth_hdr *)p->payload;
    arp_hdr = (struct etharp_hdr *)((unsigned char *)ethhdr + SIZEOF_ETH_HDR);

    memcpy(ethhdr->dest.addr, arp, 6);
    memcpy(ethhdr->src.addr, acd_netif->hwaddr, 6);
    ethhdr->type = PP_HTONS(ETHTYPE_ARP);

    memcpy(arp_hdr, arp + ETHHDR_SIZE, ARPHDR_SIZE);

    acd_netif->linkoutput(acd_netif, p);
    pbuf_free(p);
}

static int32_t EIPACD_readARP(unsigned char *buf)
{
    struct pbuf *p = NULL;
    int size = 0;

    SYS_ARCH_DECL_PROTECT(lev);
    SYS_ARCH_PROTECT(lev);
    if(arpq_head != NULL)
    {
        p = arpq_head;
        arpq_head = arpq_head->next;
        if(arpq_head == NULL)
        {
            arpq_tail = NULL;
        }
    }
    SYS_ARCH_UNPROTECT(lev);

    if(p != NULL)
    {
        size = p->tot_len;
        if(size > BUFSIZE)
        {
            size = BUFSIZE;
        }
        pbuf_copy_partial(p, buf, size, 0);
        pbuf_free(p);
    }

    return size;
}

static void EIPACD_acdMonitor(void *arg)
{
    ACD_Object *acdObj = (ACD_Object *)arg;
    unsigned char arp[ARPHDR_SIZE + ETHHDR_SIZE];
    uint32_t msg;
    void *msg_ptr;

    sys_timeout(ACDPERIOD, acd_timer_callback, NULL);

    while(1)
    {
        if(sys_mbox_tryfetch(acdObj->mbox, &msg_ptr) == ERR_OK)
        {
            msg = (uint32_t)(uintptr_t)msg_ptr;

            switch(msg)
            {
                case PORT1_MSG_LINKUP:
                    if(startupMode == EIP_STARTUP_DHCP)
                    {
                        sys_msleep(6000);
                    }

                    ACD_Link(0, 1);
                    acdEventMissed = 0;
                    break;

                case PORT1_MSG_LINKDOWN:
                    ACD_Link(0, 0);
                    break;

                case PORT2_MSG_LINKUP:
                    if(startupMode == EIP_STARTUP_DHCP)
                    {
                        sys_msleep(6000);
                    }

                    ACD_Link(1, 1);
                    acdEventMissed = 0;
                    break;

                case PORT2_MSG_LINKDOWN:
                    ACD_Link(1, 0);
                    break;

                case PORTS_MSG_LINKDOWN:
                    if(eipAppStat)
                    {
                        acdCablePull = 1;
                        ACD_Link(0, 0);
                        ACD_Link(1, 0);
                        ACD_Stop();
                        acdEIPStatus = EIP_ACD_RESTART;
                    }
                    else
                    {
                        acdEventMissed = 1;
                    }
                    break;

                case MSG_ARP:
                    EIPACD_readARP(arp);
                    ACD_RcvArpFrame(ARPHDR_SIZE + ETHHDR_SIZE, arp);
                    break;

                case MSG_NETSTOP:
                    return;

                default:
                    break;
            }
        }

        if(acdEventMissed)
        {
            if(eipAppStat && (!linkDownStat))
            {
                acdCablePull = 1;
                ACD_Link(0, 0);
                ACD_Link(1, 0);
                ACD_Stop();
                acdEIPStatus = EIP_ACD_RESTART;
                acdEventMissed = 0;
            }
        }

        sys_msleep(1);
    }
}

static void acd_timer_callback(void *arg)
{
    ACD_IncTick();
    sys_timeout(ACDPERIOD, acd_timer_callback, NULL);
}

void EIPACD_init(struct netif *netif)
{
    acd_netif = netif;

    if(!ACD_Init())
    {
        return;
    }
}

void EIPACD_exit(void)
{
    ACD_Exit();
}

void EIPACD_registerLinkCallBack(ICSSEMAC_Handle icssEmacHandle)
{
    ICSS_EmacRegisterPort1ISRCallback(icssEmacHandle,
                                      (ICSS_EmacCallBack)EIPACD_linkStatusPort1,
                                      (void *)icssEmacHandle);
    ICSS_EmacRegisterPort0ISRCallback(icssEmacHandle,
                                      (ICSS_EmacCallBack)EIPACD_linkStatusPort0,
                                      (void *)icssEmacHandle);
}

void EIPACD_deregisterLinkCallBack(ICSSEMAC_Handle icssEmacHandle)
{
    ICSS_EmacRegisterPort0ISRCallback(icssEmacHandle,
                                      (ICSS_EmacCallBack)EIP_DLR_TIMESYNC_port0ProcessLinkBrk,
                                      (void *)(icssEmacHandle));
    ICSS_EmacRegisterPort1ISRCallback(icssEmacHandle,
                                      (ICSS_EmacCallBack)EIP_DLR_TIMESYNC_port1ProcessLinkBrk,
                                      (void *)(icssEmacHandle));
}

int EIPACD_start(void)
{
    acdObj.mbox = sys_mbox_new(NUMMSGS);
    if(acdObj.mbox == NULL)
    {
        return 0;
    }

    acdObj.validSem = sys_sem_new(0);
    if(acdObj.validSem == NULL)
    {
        sys_mbox_free(acdObj.mbox);
        return 0;
    }

    acdObj.thread = sys_thread_new("UserACD_Monitor", EIPACD_acdMonitor, &acdObj, 0, 0);
    if(acdObj.thread == NULL)
    {
        sys_sem_free(acdObj.validSem);
        sys_mbox_free(acdObj.mbox);
        return 0;
    }

    EIPACD_registerLinkCallBack(eipHandle->emacHandle);

    EIPACD_updateLinkStatus(((ICSS_EmacObject *)(eipHandle->emacHandle)->object)->linkStatus[0], 0);
    EIPACD_updateLinkStatus(((ICSS_EmacObject *)(eipHandle->emacHandle)->object)->linkStatus[1], 1);

    acdStart = 1;

    return 1;
}

void EIPACD_stop(void)
{
    uint32_t msg = MSG_NETSTOP;

    EIPACD_deregisterLinkCallBack(eipHandle->emacHandle);

    if(acdObj.mbox != NULL)
    {
        sys_mbox_trypost(acdObj.mbox, (void *)(uintptr_t)msg);
    }

    sys_msleep(100);

    if(acdObj.mbox != NULL)
    {
        sys_mbox_free(acdObj.mbox);
        acdObj.mbox = NULL;
    }

    if(acdObj.validSem != NULL)
    {
        sys_sem_free(acdObj.validSem);
        acdObj.validSem = NULL;
    }

    memset(&acdObj, 0, sizeof(acdObj));

    if(eipHandle != NULL && eipHandle->emacHandle != NULL)
    {
        ((ICSS_EmacObject*)(eipHandle->emacHandle)->object)->port0ISRCall = NULL;
        ((ICSS_EmacObject*)(eipHandle->emacHandle)->object)->port0ISRUser = NULL;
        ((ICSS_EmacObject*)(eipHandle->emacHandle)->object)->port1ISRCall = NULL;
        ((ICSS_EmacObject*)(eipHandle->emacHandle)->object)->port1ISRUser = NULL;
    }

    acdStart = 0;
}

static void EIPACD_signalAddress(int flag)
{
    validAddress = flag;
    if(acdObj.validSem != NULL)
    {
        sys_sem_signal(acdObj.validSem);
    }
}

int EIPACD_getACDLEDStat(void)
{
    return ledACD;
}

int EIPACD_event(uint32_t dwEvent)
{
    uint32_t event = dwEvent & 0xffff;

    switch(event)
    {
        case ACD_EVENT_IP_LOST:
            acdARPFlag = USE_ACD_ARP;
            ledACD = ACD_LED_CONFLICT;
            EIPACD_signalAddress(0);
            ACD_Link(0, 0);
            ACD_Link(1, 0);
            ACD_Stop();
            ACD_Exit();
            EIPACD_stop();
            acdEIPStatus = EIP_ACD_STOP;
            break;

        case ACD_EVENT_IP_ANNOUNCED:
            acdARPFlag = USE_LWIP_ARP;
            EIPACD_signalAddress(1);
            acdEIPStatus = EIP_ACD_START;
            break;

        case ACD_EVENT_LINK_INTEGRITY:
            break;

        case ACD_EVENT_DEFENSE:
            acdARPFlag = USE_LWIP_ARP;
            break;

        case ACD_EVENT_ON_GOING_DETECTION:
            acdARPFlag = USE_LWIP_ARP;
            break;

        case ACD_EVENT_NO_LINK:
            break;

        default:
            break;
    }

    return 1;
}

int EIPACD_conflictDetected(ST_ACD_CONFLICT_DETECTED stAcdConflictDetected)
{
    EIP_flashWrite(SPI_EEPROM_ACD_CONFLICT_OFFSET,
                   (uint8_t *) & (stAcdConflictDetected.byAcdActivity),
                   sizeof(ST_ACD_CONFLICT_DETECTED));

    return 1;
}

int EIPACD_sendArpFrame(unsigned short wFrameSize, unsigned char *byFrame)
{
    countACD++;

    EIPACD_sendLwIPArpPacket(byFrame, 0);
    return 1;
}

int EIPACD_sendArpFrameUnicast(unsigned short wFrameSize, unsigned char *byFrame)
{
    countACD++;

    EIPACD_sendLwIPArpPacketUnicast(byFrame, 0);
    return 1;
}

err_t EIPACD_netif_input(struct pbuf *p, struct netif *inp)
{
    struct eth_hdr *ethhdr;
    uint16_t type;

    if(p->len < SIZEOF_ETH_HDR)
    {
        pbuf_free(p);
        return ERR_ARG;
    }

    ethhdr = (struct eth_hdr *)p->payload;
    type = lwip_ntohs(ethhdr->type);

    if(type == ETHTYPE_ARP)
    {
        if(acdStart)
        {
            EIPACD_arp_input(p, inp);
        }
        
        if(acdARPFlag == USE_LWIP_ARP)
        {
            return etharp_input(p, inp);
        }
        else
        {
            pbuf_free(p);
            return ERR_OK;
        }
    }

    return ERR_ARG;
}

err_t EIPACD_netif_output(struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    if(acdARPFlag == USE_ACD_ARP)
    {
        pbuf_free(p);
        return ERR_OK;
    }

    return etharp_output(netif, p, ipaddr);
}

