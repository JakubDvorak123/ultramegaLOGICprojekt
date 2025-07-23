#include "Logic.hpp"
#include <iostream>
#include <array>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
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
#define SERVER_PORT 80
#define BROADCAST_PORT 3333
#define DISCOVERY_PORT 3334

static const char *TAG = "wifi_logic";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static int s_retry_num = 0;
static esp_ip4_addr_t s_ip_addr;
static esp_ip4_addr_t e_ip_addr;
static httpd_handle_t server = NULL;
static TaskHandle_t broadcast_task_handle = NULL;
static TaskHandle_t discovery_task_handle = NULL;

// Struktura pro zpravy mezi zarizeni
typedef struct {
    char device_name[32];
    char device_type[16];
    uint32_t ip_address;
    uint16_t http_port;
    uint32_t uptime_ms;
} device_info_t;

// Task pro posilani broadcast zprav (oznamuje pritomnost tohoto zarizeni)
static void broadcast_task(void *pvParameters)
{
    int sock;
    struct sockaddr_in dest_addr;
    device_info_t device_info;
    
    // Priprava informaci o zarizeni
    snprintf(device_info.device_name, sizeof(device_info.device_name), "ESP32-Logic-%08X", (unsigned int)esp_random());
    strcpy(device_info.device_type, "Logic_Board");
    device_info.ip_address = s_ip_addr.addr;
    device_info.http_port = SERVER_PORT;
    
    while (1) {
        // Vytvoreni UDP socketu
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Nelze vytvorit broadcast socket\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        // Povoleni broadcast
        int broadcast_enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
            printf("Nelze povolit broadcast\n");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        // Nastaveni broadcast adresy
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(BROADCAST_PORT);
        dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        
        // Aktualizace uptime
        device_info.uptime_ms = esp_timer_get_time() / 1000;
        
        // Odeslani broadcast zpravy
        int err = sendto(sock, &device_info, sizeof(device_info), 0, 
                        (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            printf("Chyba pri odesilani broadcast zpravy\n");
        } else {
            printf("Broadcast zprava odeslana: %s na IP " IPSTR "\n", 
                   device_info.device_name, IP2STR(&s_ip_addr));
        }
        
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(10000)); // Odesilat kazdych 10 sekund
    }
}

// Task pro naslouchani broadcast zpram (objevuje ostatni zarizeni)
static void discovery_task(void *pvParameters)
{
    int sock;
    struct sockaddr_in listen_addr, source_addr;
    socklen_t socklen = sizeof(source_addr);
    device_info_t received_info;
    
    while (1) {
        // Vytvoreni UDP socketu pro naslouchani
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            printf("Nelze vytvorit discovery socket\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        // Povoleni znovu pouziti adresy
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        
        // Nastaveni adresy pro naslouchani
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(BROADCAST_PORT);
        listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        
        // Bind socket
        if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
            printf("Nelze bind discovery socket\n");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        printf("Nasloucham broadcast zpravám na portu %d\n", BROADCAST_PORT);
        
        while (1) {
            // Prijmout zpravy od ostatnich zarizeni
            int len = recvfrom(sock, &received_info, sizeof(received_info), 0,
                              (struct sockaddr *)&source_addr, &socklen);
            
            if (len > 0) {
                // Ignorovat zpravy od sebe sama
                if (received_info.ip_address != s_ip_addr.addr) {
                    struct in_addr addr;
                    addr.s_addr = received_info.ip_address;
                    
                    printf("=== OBJEVENO NOVE ZARIZENI ===\n");
                    printf("Nazev: %s\n", received_info.device_name);
                    printf("Typ: %s\n", received_info.device_type);
                    printf("IP: %s\n", inet_ntoa(addr));
                    printf("HTTP port: %d\n", received_info.http_port);
                    printf("Uptime: %lu ms\n", received_info.uptime_ms);
                    printf("URL: http://%s:%d\n", inet_ntoa(addr), received_info.http_port);
                    printf("=============================\n");
                }
            } else if (len < 0) {
                printf("Chyba pri prijimani broadcast zpravy\n");
                break;
            }
        }
        
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Spusteni discovery systemu
static void start_device_discovery(void)
{
    // Spustit task pro odesilani broadcast zprav
    xTaskCreate(broadcast_task, "broadcast_task", 4096, NULL, 5, &broadcast_task_handle);
    
    // Spustit task pro naslouchani broadcast zpravám
    xTaskCreate(discovery_task, "discovery_task", 4096, NULL, 5, &discovery_task_handle);
    
    printf("Device discovery system spusten\n");
}

// HTTP handler pro hlavni stranku
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char resp_str[512];
    snprintf(resp_str, sizeof(resp_str),
        "<html><body>"
        "<h1>ESP32 Logic Board Server</h1>"
        "<p>IP adresa: " IPSTR "</p>"
        "<p>Port: %d</p>"
        "<p>Program bezi uspesne!</p>"
        "<p><a href='/info'>Zobrazit informace o systemu</a></p>"
        "<p><a href='/devices'>Objevena zarizeni</a></p>"
        "<p><strong>Device Discovery:</strong> Toto zarizeni automaticky vysilá broadcast zpravy a naslouchá ostatnim zarizenım na síti.</p>"
        "</body></html>",
        IP2STR(&s_ip_addr), SERVER_PORT);
    
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP handler pro systemove informace
static esp_err_t info_get_handler(httpd_req_t *req)
{
    char resp_str[1024];
    snprintf(resp_str, sizeof(resp_str),
        "<html>"
        "<head><title>Systemove informace</title>"
        "<script src=\"https://cdn.jsdelivr.net/npm/@tailwindcss/browser@4\"></script>"
        "</head>"
        "<body>"
        "<h1>Systemove informace</h1>"
        "<p>IP adresa: " IPSTR "</p>"
        "<p>Free heap: %lu bytes</p>"
        "<p>Uptime: %lld ms</p>"
        "<p>WiFi SSID: %s</p>"
        "<p><a href='/'>Zpet na hlavni stranku</a></p>"
        "</body></html>",
        IP2STR(&s_ip_addr),
        esp_get_free_heap_size(),
        esp_timer_get_time() / 1000,
        WIFI_SSID);
    
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP handler pro stranku s objevenymi zarizeni
static esp_err_t devices_get_handler(httpd_req_t *req)
{
    char resp_str[1024];
    snprintf(resp_str, sizeof(resp_str),
        "<html>"
        "<head><title>Objevena zarizeni</title></head>"
        "<body>"
        "<h1>Device Discovery</h1>"
        "<p>Toto zarizeni automaticky vysilá broadcast zpravy kazdych 10 sekund na portu %d</p>"
        "<p>Sledujte serial monitor pro informace o objevenych zarizenich.</p>"
        "<p><strong>Jak to funguje:</strong></p>"
        "<ul>"
        "<li>Kazdé ESP32 zarizeni vysilá broadcast zpravy se svymi informacemi</li>"
        "<li>Ostatni zarizeni tyto zpravy prijimaji a zobrazuji v serial monitoru</li>"
        "<li>Automaticky se objevuji nova zarizeni na síti</li>"
        "</ul>"
        "<p><a href='/'>Zpet na hlavni stranku</a></p>"
        "<p><a href='/info'>Systemove informace</a></p>"
        "<script>"
        "setTimeout(function(){ location.reload(); }, 5000);"  // Auto refresh kazdych 5 sekund
        "</script>"
        "</body></html>",
        BROADCAST_PORT);
    
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Konfigurace HTTP endpoints
static const httpd_uri_t hello = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t info = {
    .uri       = "/info",
    .method    = HTTP_GET,
    .handler   = info_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t devices = {
    .uri       = "/devices",
    .method    = HTTP_GET,
    .handler   = devices_get_handler,
    .user_ctx  = NULL
};

// Spusteni HTTP serveru
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = SERVER_PORT;
    config.lru_purge_enable = true;

    printf("Spoustim HTTP server na portu %d\n", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        printf("HTTP server uspesne spusten\n");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &info);
        httpd_register_uri_handler(server, &devices);
        return server;
    }

    printf("Chyba pri spousteni HTTP serveru!\n");
    return NULL;
}

// Zastaveni HTTP serveru
static esp_err_t stop_webserver(httpd_handle_t server)
{
    return httpd_stop(server);
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
        
        // Spustit HTTP server
        server = start_webserver();
        if (server) {
            printf("HTTP server dostupny na: http://" IPSTR ":%d\n", 
                   IP2STR(&s_ip_addr), SERVER_PORT);
        }
        
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
        printf("HTTP server bezi na portu: %d\n", SERVER_PORT);
        printf("Pristup pres webovy prohlizec: http://" IPSTR ":%d\n", 
               IP2STR(&s_ip_addr), SERVER_PORT);

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

void logicMain()
{
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


    // Hlavni smycka programu
    while (true)
    {
        if (server) {
            printf("HTTP server bezi na http://" IPSTR ":%d - Free heap: %lu bytes\n", 
                   IP2STR(&s_ip_addr), SERVER_PORT, esp_get_free_heap_size());
        } else {
            printf("WiFi program bezi (bez HTTP serveru)...\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Cekani 10 sekund
    }
}