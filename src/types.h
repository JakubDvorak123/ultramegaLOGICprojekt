#pragma once
#include <esp_netif_ip_addr.h>

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

struct game_state_t
{
    char foo[64];  // Fixed-size buffer instead of std::string
    char bar[64];  // Fixed-size buffer instead of std::string
    int score;
};