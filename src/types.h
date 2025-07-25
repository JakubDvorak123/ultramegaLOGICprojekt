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
    char action[32];     // "shoot", "hit", "miss", "ready", "game_over"
    int target_x;        // Target coordinates for shooting
    int target_y;
    int score;           // Player score
    bool is_hit;         // Was the shot a hit?
    bool game_over;      // Is the game finished?
    char message[64];    // Additional game message
};