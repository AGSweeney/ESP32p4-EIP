#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_phy.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lwip/netif.h"
#include "lwip/err.h"
#include "lwip/tcpip.h"
#include "lwip/ip4_addr.h"
#include "lwip/acd.h"
#include "lwip/etharp.h"
#include "nvs_flash.h"
#include "opener.h"
#include "nvtcpip.h"
#include "ciptcpipinterface.h"
#include "sdkconfig.h"
#include "esp_netif_net_stack.h"

void SampleApplicationSetActiveNetif(struct netif *netif);
void SampleApplicationNotifyLinkUp(void);
void SampleApplicationNotifyLinkDown(void);

static const char *TAG = "opener_main";
static struct netif *s_netif = NULL;
static SemaphoreHandle_t s_netif_mutex = NULL;
#if LWIP_IPV4 && LWIP_ACD
static struct acd s_static_ip_acd;
static bool s_acd_registered = false;
static SemaphoreHandle_t s_acd_sem = NULL;
static acd_callback_enum_t s_acd_last_state = ACD_RESTART_CLIENT;
static bool s_acd_probe_pending = false;
static esp_netif_ip_info_t s_pending_static_ip_cfg = {0};
#endif

static bool tcpip_config_uses_dhcp(void);
static void configure_hostname(esp_netif_t *netif);
static void opener_configure_dns(esp_netif_t *netif);

static bool tcpip_config_uses_dhcp(void) {
    return (g_tcpip.config_control & kTcpipCfgCtrlMethodMask) == kTcpipCfgCtrlDhcp;
}

static bool tcpip_static_config_valid(void) {
    if ((g_tcpip.config_control & kTcpipCfgCtrlMethodMask) != kTcpipCfgCtrlStaticIp) {
        return true;
    }
    return CipTcpIpIsValidNetworkConfig(&g_tcpip.interface_configuration);
}

static void configure_hostname(esp_netif_t *netif) {
    if (g_tcpip.hostname.length > 0 && g_tcpip.hostname.string != NULL) {
        size_t length = g_tcpip.hostname.length;
        if (length > 63) {
            length = 63;
        }
        char host[64];
        memcpy(host, g_tcpip.hostname.string, length);
        host[length] = '\0';
        esp_netif_set_hostname(netif, host);
    }
}

static void opener_configure_dns(esp_netif_t *netif) {
    esp_netif_dns_info_t dns_info = {
        .ip.type = IPADDR_TYPE_V4,
        .ip.u_addr.ip4.addr = g_tcpip.interface_configuration.name_server
    };
    if (dns_info.ip.u_addr.ip4.addr != 0) {
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info));
    }

    dns_info.ip.u_addr.ip4.addr = g_tcpip.interface_configuration.name_server_2;
    if (dns_info.ip.u_addr.ip4.addr != 0) {
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info));
    }
}

#if LWIP_IPV4 && LWIP_ACD
typedef struct {
    struct netif *netif;
    ip4_addr_t ip;
    err_t err;
} AcdStartContext;

static bool netif_has_valid_hwaddr(struct netif *netif) {
    if (netif == NULL) {
        return false;
    }
    if (netif->hwaddr_len != ETH_HWADDR_LEN) {
        return false;
    }
    for (int i = 0; i < ETH_HWADDR_LEN; ++i) {
        if (netif->hwaddr[i] != 0) {
            return true;
        }
    }
    return false;
}

static void tcpip_acd_conflict_callback(struct netif *netif, acd_callback_enum_t state) {
    (void)netif;
    s_acd_last_state = state;
    switch (state) {
        case ACD_IP_OK:
            g_tcpip.status &= ~(kTcpipStatusAcdStatus | kTcpipStatusAcdFault);
            break;
        default:
            g_tcpip.status |= kTcpipStatusAcdStatus;
            g_tcpip.status |= kTcpipStatusAcdFault;
            break;
    }
    if (s_acd_sem != NULL) {
        xSemaphoreGive(s_acd_sem);
    }
}

static void tcpip_acd_start_cb(void *arg) {
    AcdStartContext *ctx = (AcdStartContext *)arg;
    ctx->err = ERR_OK;
    if (!s_acd_registered) {
        if (acd_add(ctx->netif, &s_static_ip_acd, tcpip_acd_conflict_callback) == ERR_OK) {
            s_acd_registered = true;
        } else {
            ctx->err = ERR_IF;
            return;
        }
    }
    acd_stop(&s_static_ip_acd);
    ctx->err = acd_start(ctx->netif, &s_static_ip_acd, ctx->ip);
}

static void tcpip_acd_stop_cb(void *arg) {
    (void)arg;
    acd_stop(&s_static_ip_acd);
}

static bool tcpip_perform_acd(struct netif *netif, const ip4_addr_t *ip) {
    if (!g_tcpip.select_acd) {
        g_tcpip.status &= ~(kTcpipStatusAcdStatus | kTcpipStatusAcdFault);
        CipTcpIpSetLastAcdActivity(0);
        return true;
    }

    if (netif == NULL) {
        ESP_LOGW(TAG, "ACD requested but no netif available");
        g_tcpip.status |= kTcpipStatusAcdStatus | kTcpipStatusAcdFault;
        CipTcpIpSetLastAcdActivity(3);
        return false;
    }

    if (s_acd_sem == NULL) {
        s_acd_sem = xSemaphoreCreateBinary();
        if (s_acd_sem == NULL) {
            ESP_LOGE(TAG, "Failed to create ACD semaphore");
            g_tcpip.status |= kTcpipStatusAcdStatus | kTcpipStatusAcdFault;
            CipTcpIpSetLastAcdActivity(3);
            return false;
        }
    }

    while (xSemaphoreTake(s_acd_sem, 0) == pdTRUE) {
        /* flush any stale signals */
    }

    s_acd_last_state = ACD_RESTART_CLIENT;
    CipTcpIpSetLastAcdActivity(2);

    AcdStartContext ctx = {
        .netif = netif,
        .ip = *ip,
        .err = ERR_OK,
    };

    if (tcpip_callback_with_block(tcpip_acd_start_cb, &ctx, 1) != ERR_OK || ctx.err != ERR_OK) {
        ESP_LOGE(TAG, "Failed to start ACD probe (err=%d)", (int)ctx.err);
        g_tcpip.status |= kTcpipStatusAcdStatus | kTcpipStatusAcdFault;
        CipTcpIpSetLastAcdActivity(3);
        return false;
    }

    if (xSemaphoreTake(s_acd_sem, pdMS_TO_TICKS(10000)) != pdTRUE) {
        ESP_LOGE(TAG, "ACD probe timed out");
        tcpip_callback_with_block(tcpip_acd_stop_cb, NULL, 1);
        g_tcpip.status |= kTcpipStatusAcdStatus | kTcpipStatusAcdFault;
        CipTcpIpSetLastAcdActivity(3);
        return false;
    }

    if (s_acd_last_state == ACD_IP_OK) {
        CipTcpIpSetLastAcdActivity(0);
        return true;
    }

    tcpip_callback_with_block(tcpip_acd_stop_cb, NULL, 1);
    if (s_acd_last_state == ACD_DECLINE) {
        ESP_LOGE(TAG, "ACD declined IP address");
    } else {
        ESP_LOGE(TAG, "ACD reported conflict (state=%d)", (int)s_acd_last_state);
    }
    CipTcpIpSetLastAcdActivity(3);
    return false;
}

static void tcpip_try_pending_acd(esp_netif_t *netif, struct netif *lwip_netif) {
    if (!s_acd_probe_pending || netif == NULL || lwip_netif == NULL) {
        return;
    }
    if (!netif_has_valid_hwaddr(lwip_netif)) {
        return;
    }
    if (!netif_is_link_up(lwip_netif)) {
        return;
    }

    ip4_addr_t desired_ip = { .addr = s_pending_static_ip_cfg.ip.addr };
    CipTcpIpSetLastAcdActivity(2);
    if (!tcpip_perform_acd(lwip_netif, &desired_ip)) {
        ESP_LOGE(TAG, "Deferred ACD conflict detected for " IPSTR, IP2STR(&s_pending_static_ip_cfg.ip));
        ESP_LOGW(TAG, "Static configuration remains active despite ACD fault");
    }

    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &s_pending_static_ip_cfg));
    opener_configure_dns(netif);
    s_acd_probe_pending = false;
    CipTcpIpSetLastAcdActivity(0);
}
#else
static bool tcpip_perform_acd(struct netif *netif, const ip4_addr_t *ip) {
    (void)netif;
    (void)ip;
    if (g_tcpip.select_acd) {
        ESP_LOGW(TAG, "ACD requested but not supported by lwIP configuration");
    }
    g_tcpip.status &= ~(kTcpipStatusAcdStatus | kTcpipStatusAcdFault);
    return true;
}
#endif

static void configure_netif_from_tcpip(esp_netif_t *netif) {
    if (netif == NULL) {
        return;
    }

    struct netif *lwip_netif = (struct netif *)esp_netif_get_netif_impl(netif);

    if (tcpip_config_uses_dhcp()) {
        esp_netif_dhcpc_stop(netif);
        esp_netif_dhcpc_start(netif);
    } else {
        esp_netif_ip_info_t ip_info = {0};
        ip_info.ip.addr = g_tcpip.interface_configuration.ip_address;
        ip_info.netmask.addr = g_tcpip.interface_configuration.network_mask;
        ip_info.gw.addr = g_tcpip.interface_configuration.gateway;
        esp_netif_dhcpc_stop(netif);

#if LWIP_IPV4 && LWIP_ACD
        s_pending_static_ip_cfg = ip_info;
        s_acd_probe_pending = true;
        CipTcpIpSetLastAcdActivity(1);
        if (g_tcpip.select_acd && lwip_netif != NULL && netif_is_link_up(lwip_netif)) {
            tcpip_try_pending_acd(netif, lwip_netif);
        }
        if (!g_tcpip.select_acd) {
            CipTcpIpSetLastAcdActivity(0);
            s_acd_probe_pending = false;
            ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
            opener_configure_dns(netif);
        }
#else
        ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
        opener_configure_dns(netif);
#endif
    }

    configure_hostname(netif);
    g_tcpip.status |= 0x01;
    g_tcpip.status &= ~kTcpipStatusIfaceCfgPend;
}

static void ethernet_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    esp_netif_t *eth_netif = (esp_netif_t *)arg;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
               mac_addr[0], mac_addr[1], mac_addr[2],
               mac_addr[3], mac_addr[4], mac_addr[5]);
        ESP_ERROR_CHECK(esp_netif_set_mac(eth_netif, mac_addr));
        #if LWIP_IPV4 && LWIP_ACD
        if (!tcpip_config_uses_dhcp()) {
            struct netif *lwip_netif = (struct netif *)esp_netif_get_netif_impl(eth_netif);
            tcpip_try_pending_acd(eth_netif, lwip_netif);
        }
        #endif
        SampleApplicationNotifyLinkUp();
        if (!tcpip_config_uses_dhcp() && eth_netif != NULL) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(eth_netif, &ip_info) == ESP_OK) {
                ip_event_got_ip_t event = { .esp_netif = eth_netif };
                event.ip_info = ip_info;
                esp_event_post(IP_EVENT, IP_EVENT_ETH_GOT_IP, &event, sizeof(event), portMAX_DELAY);
            }
        }
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        #if LWIP_IPV4 && LWIP_ACD
        tcpip_callback_with_block(tcpip_acd_stop_cb, NULL, 1);
        #endif
        SampleApplicationNotifyLinkDown();
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    
    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    
    // Create mutex on first call if needed
    if (s_netif_mutex == NULL) {
        s_netif_mutex = xSemaphoreCreateMutex();
        if (s_netif_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create netif mutex");
            return;
        }
    }
    
    // Take mutex to protect s_netif access
    if (xSemaphoreTake(s_netif_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take netif mutex");
        return;
    }
    
    if (s_netif == NULL) {
        for (struct netif *netif = netif_list; netif != NULL; netif = netif->next) {
            if (netif_is_up(netif) && netif_is_link_up(netif)) {
                s_netif = netif;
                break;
            }
        }
    }
    
    struct netif *netif_to_use = s_netif;
    xSemaphoreGive(s_netif_mutex);
    
    if (netif_to_use != NULL) {
        SampleApplicationSetActiveNetif(netif_to_use);
        opener_init(netif_to_use);
        SampleApplicationNotifyLinkUp();
    } else {
        ESP_LOGE(TAG, "Failed to find netif");
    }
}

void app_main(void)
{
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);
    (void)NvTcpipLoad(&g_tcpip);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Ensure default configuration uses DHCP when nothing stored */
    if ((g_tcpip.config_control & kTcpipCfgCtrlMethodMask) != kTcpipCfgCtrlStaticIp &&
        (g_tcpip.config_control & kTcpipCfgCtrlMethodMask) != kTcpipCfgCtrlDhcp) {
        g_tcpip.config_control &= ~kTcpipCfgCtrlMethodMask;
        g_tcpip.config_control |= kTcpipCfgCtrlDhcp;
    }
    if (!tcpip_static_config_valid()) {
        ESP_LOGW(TAG, "Invalid static configuration detected, switching to DHCP");
        g_tcpip.config_control &= ~kTcpipCfgCtrlMethodMask;
        g_tcpip.config_control |= kTcpipCfgCtrlDhcp;
        g_tcpip.interface_configuration.ip_address = 0;
        g_tcpip.interface_configuration.network_mask = 0;
        g_tcpip.interface_configuration.gateway = 0;
        g_tcpip.interface_configuration.name_server = 0;
        g_tcpip.interface_configuration.name_server_2 = 0;
        g_tcpip.status &= ~(kTcpipStatusAcdStatus | kTcpipStatusAcdFault);
        NvTcpipStore(&g_tcpip);
    }
    if (g_tcpip.select_acd) {
        ESP_LOGW(TAG, "ACD selection stored in NV; disabling at boot");
        g_tcpip.select_acd = false;
        g_tcpip.status &= ~(kTcpipStatusAcdStatus | kTcpipStatusAcdFault);
        NvTcpipStore(&g_tcpip);
    }
    if (tcpip_config_uses_dhcp()) {
        g_tcpip.interface_configuration.ip_address = 0;
        g_tcpip.interface_configuration.network_mask = 0;
        g_tcpip.interface_configuration.gateway = 0;
        g_tcpip.interface_configuration.name_server = 0;
        g_tcpip.interface_configuration.name_server_2 = 0;
    }

    g_tcpip.status |= 0x01;
    g_tcpip.status &= ~kTcpipStatusIfaceCfgPend;

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    ESP_ERROR_CHECK(esp_netif_set_default_netif(eth_netif));

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, 
                                               &ethernet_event_handler, eth_netif));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, 
                                               &got_ip_event_handler, eth_netif));

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    
    phy_config.phy_addr = CONFIG_OPENER_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_OPENER_ETH_PHY_RST_GPIO;

    esp32_emac_config.smi_gpio.mdc_num = CONFIG_OPENER_ETH_MDC_GPIO;
    esp32_emac_config.smi_gpio.mdio_num = CONFIG_OPENER_ETH_MDIO_GPIO;

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

    esp_eth_netif_glue_handle_t glue = esp_eth_new_netif_glue(eth_handle);
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, glue));

    configure_netif_from_tcpip(eth_netif);
    
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

