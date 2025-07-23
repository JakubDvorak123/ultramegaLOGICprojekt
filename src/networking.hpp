#include "Logic.hpp"
#include <iostream>
#include <array>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_SSID "ChataULesa"
#define WIFI_PASS "ulesa529"
#define WIFI_MAXIMUM_RETRY 5
#define BROADCAST_PORT 3333

static const char *TAG = "wifi_logic";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;
static esp_ip4_addr_t s_ip_addr;
static esp_ip4_addr_t enemy_ip_addr;
static TaskHandle_t broadcast_task_handle = NULL;
static TaskHandle_t discovery_task_handle = NULL;
static bool is_master = false;
static bool connection_established = false;
// Forward declarations
static void listening_task(void *pvParameters);

struct main_data
{
    esp_ip4_addr_t s_ip_addr;  // Lokální IP adresa
    esp_ip4_addr_t enemy_ip_addr;  // IP adresa partnera
    bool is_master;  // true pokud jsme master, false pokud jsme slave
};


// Struktura pro zpravy mezi zarizeni
typedef struct {
    char device_name[32];
    char device_type[16];
    uint32_t ip_address;
    uint32_t uptime_ms;
    bool is_response;  // true pokud je to odpoved na discovery
} device_info_t;

// Task pro initial discovery - posle 5 broadcast zprav a ceka na odpoved
static void initial_discovery_task(void *pvParameters)
{
    int sock;
    struct sockaddr_in dest_addr, listen_addr, source_addr;
    socklen_t socklen = sizeof(source_addr);
    device_info_t device_info, received_info;
    
    // Priprava informaci o zarizeni
    snprintf(device_info.device_name, sizeof(device_info.device_name), "ESP32-Logic-%08X", (unsigned int)esp_random());
    strcpy(device_info.device_type, "Logic_Board");
    device_info.ip_address = s_ip_addr.addr;
    device_info.is_response = false;  // Toto je discovery zprava
    
    printf("=== INITIAL DISCOVERY - Hledam jine ESP32 ===\n");
    
    // Pokus o 5 broadcast zprav
    for (int attempt = 0; attempt < 5; attempt++) {
        printf("Discovery pokus %d/5\n", attempt + 1);
        
        // Vytvoreni UDP socketu pro broadcast
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Nelze vytvorit discovery socket\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        // Povoleni broadcast
        int broadcast_enable = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
        
        // Nastavit timeout pro receive
        struct timeval timeout;
        timeout.tv_sec = 2;  // 2 sekundy timeout
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        // Bind pro naslouchani odpovedi
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(BROADCAST_PORT);
        listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
        
        // Nastaveni broadcast adresy
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(BROADCAST_PORT);
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        
        // Aktualizace uptime
        device_info.uptime_ms = esp_timer_get_time() / 1000;
        
        // Odeslani discovery zpravy
        int err = sendto(sock, &device_info, sizeof(device_info), 0, 
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        
        if (err >= 0) {
            printf("Discovery zprava odeslana, cekam na odpoved...\n");
            
            // Cekat na odpoved
            int len = recvfrom(sock, &received_info, sizeof(received_info), 0,
                              (struct sockaddr *)&source_addr, &socklen);
            
            if (len > 0 && received_info.ip_address != s_ip_addr.addr && received_info.is_response) {
                // Dostali jsme odpoved od jineho ESP32!
                enemy_ip_addr.addr = received_info.ip_address;
                is_master = true;
                connection_established = true;
                
                printf("=== USPECH! NALEZEN PARTNER ===\n");
                printf("Jsem MASTER\n");
                printf("Partner IP: " IPSTR "\n", IP2STR(&enemy_ip_addr));
                printf("Partner: %s\n", received_info.device_name);
                printf("===============================\n");
                
                close(sock);
                vTaskDelete(NULL);  // Ukoncit tento task
                return;
            }
        }
        
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(1000));  // Cekat 1 sekundu mezi pokusy
    }
    
    // Zadna odpoved po 5 pokusech - stanem se slave (listener)
    printf("=== ZADNA ODPOVED - STANEM SE SLAVE ===\n");
    is_master = false;
    
    // Spustit listening task
    xTaskCreate(listening_task, "listening_task", 4096, NULL, 5, &discovery_task_handle);
    
    vTaskDelete(NULL);  // Ukoncit tento task
}

// Task pro naslouchani jako slave - ceka na discovery zpravy a odpovida
static void listening_task(void *pvParameters)
{
    int sock;
    struct sockaddr_in listen_addr, source_addr;
    socklen_t socklen = sizeof(source_addr);
    device_info_t received_info, response_info;
    
    printf("=== SLAVE MODE - Nasloucham discovery zpravám ===\n");
    
    // Priprava response informaci
    snprintf(response_info.device_name, sizeof(response_info.device_name), "ESP32-Logic-%08X", (unsigned int)esp_random());
    strcpy(response_info.device_type, "Logic_Board");
    response_info.ip_address = s_ip_addr.addr;
    response_info.is_response = true;  // Toto je odpoved
    
    while (!connection_established) {
        // Vytvoreni UDP socketu pro naslouchani
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Nelze vytvorit listening socket\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        
        // Nastaveni adresy pro naslouchani
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(BROADCAST_PORT);
        listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
            printf("Nelze bind listening socket\n");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        printf("Slave naslouchá na portu %d\n", BROADCAST_PORT);
        
        while (!connection_established) {
            // Prijmout zpravy od master zarizeni
            int len = recvfrom(sock, &received_info, sizeof(received_info), 0,
                              (struct sockaddr *)&source_addr, &socklen);
            
            if (len > 0 && received_info.ip_address != s_ip_addr.addr && !received_info.is_response) {
                // Dostali jsme discovery zpravu od jineho ESP32!
                enemy_ip_addr.addr = received_info.ip_address;
                connection_established = true;
                
                printf("=== DISCOVERY ZPRAVA PRIJATA ===\n");
                printf("Jsem SLAVE\n");
                printf("Master IP: " IPSTR "\n", IP2STR(&enemy_ip_addr));
                printf("Master: %s\n", received_info.device_name);
                
                // Poslat odpoved zpet
                response_info.uptime_ms = esp_timer_get_time() / 1000;
                
                int err = sendto(sock, &response_info, sizeof(response_info), 0,
                                (struct sockaddr *)&source_addr, socklen);
                
                if (err >= 0) {
                    printf("Odpoved odeslana zpet k master\n");
                } else {
                    printf("Chyba pri odesilani odpovedi\n");
                }
                
                printf("=============================\n");
                break;
            }
        }
        
        close(sock);
    }
    
    printf("Slave ukoncuje naslouchani - spojeni navazano\n");
    vTaskDelete(NULL);  // Ukoncit tento task
}

// Spusteni discovery systemu
static void start_device_discovery(void)
{
    printf("=== SPOUSTIM DEVICE DISCOVERY SYSTEM ===\n");
    
    // Spustit initial discovery task
    xTaskCreate(initial_discovery_task, "initial_discovery", 4096, NULL, 5, &broadcast_task_handle);
    
    printf("Initial discovery task spusten\n");
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            printf("Pokus o pripojeni k WiFi, pokus %d\n", s_retry_num);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        printf("Pripojeni k WiFi se nezdarilo\n");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        printf("Ziskana IP adresa:" IPSTR "\n", IP2STR(&event->ip_info.ip));
        
        // Ulozit IP adresu pro pouziti v HTTP serveru
        s_ip_addr = event->ip_info.ip;
        
        // Spustit device discovery system
        start_device_discovery();
        
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("WiFi inicializovano. Pripojuji se k SSID:%s...\n", WIFI_SSID);

    // Cekani na pripojeni nebo chybu
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        printf("Uspesne pripojeno k WiFi SSID:%s\n", WIFI_SSID);
        printf("Lokalni IP adresa: " IPSTR "\n", IP2STR(&s_ip_addr));
        printf("Device discovery system spusten na portu: %d\n", BROADCAST_PORT);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        printf("Pripojeni k WiFi SSID:%s se nezdarilo\n", WIFI_SSID);
    }
    else
    {
        printf("Neocekavana udalost\n");
    }
}

main_data pair_esp()
{
    main_data data;
    printf("Spoustim WiFi program...\n");

    // Inicializace NVS (povinne pro WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    printf("NVS inicializovano uspesne\n");

    // Inicializace WiFi
    wifi_init_sta();

    // Cekat na dokonceni parovani
    printf("Cekam na dokonceni parovani...\n");
    while (!connection_established) {
        vTaskDelay(pdMS_TO_TICKS(100));  // Cekat 100ms mezi kontrolami
    }
    
    printf("Parovani dokonceno!\n");
    
    data.s_ip_addr = s_ip_addr;
    data.enemy_ip_addr = enemy_ip_addr;
    data.is_master = is_master;

    return data;
}