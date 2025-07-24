# Networking.hpp Documentation

## Overview

`networking.hpp` provides a complete ESP32-to-ESP32 WiFi pairing and communication system for Logic boards. The system uses UDP broadcasting for device discovery and establishes a master-slave relationship between two ESP32 devices.

## Architecture

### Connection Flow
1. **WiFi Connection**: Both devices connect to "ChataULesa" network
2. **Discovery Phase**: First device becomes master, attempts 5 broadcasts
3. **Pairing**: If response received → master mode, else → slave mode  
4. **Communication**: Established master-slave pair can exchange game data

### Port Usage
- **Port 3333**: Device discovery and pairing
- **Port 3334**: Game data communication (`BROADCAST_PORT + 1`)

## Data Structures

### `main_data`
```cpp
struct main_data {
    esp_ip4_addr_t s_ip_addr;      // Local IP address
    esp_ip4_addr_t enemy_ip_addr;  // Partner IP address  
    bool is_master;                // true = master, false = slave
};
```

### `device_info_t` 
```cpp
typedef struct {
    char device_name[32];    // Generated device identifier
    char device_type[16];    // "Logic_Board"
    uint32_t ip_address;     // Device IP address
    uint32_t uptime_ms;      // Device uptime in milliseconds
    bool is_response;        // true = response to discovery
} device_info_t;
```

### `game_state_t`
```cpp
struct game_state_t {
    std::string foo;  // Custom game data field
    std::string bar;  // Custom game data field  
    int score;        // Game score
};
```

## Public Functions

### `main_data pair_esp()`

**Purpose**: Main pairing function that establishes connection between two ESP32 devices

**Behavior**:
- Initializes NVS flash and WiFi
- Connects to "ChataULesa" network
- Runs device discovery protocol
- Waits for pairing completion
- Returns connection details

**Returns**: `main_data` struct with local IP, partner IP, and master/slave status

**Usage Example**:
```cpp
void logicMain() {
    main_data pairing = pair_esp();
    
    printf("Paired successfully!\n");
    printf("Local IP: " IPSTR "\n", IP2STR(&pairing.s_ip_addr));
    printf("Partner IP: " IPSTR "\n", IP2STR(&pairing.enemy_ip_addr));
    printf("Role: %s\n", pairing.is_master ? "Master" : "Slave");
}
```

**Important**: This function blocks until pairing is complete or fails

---

### `bool sendData(struct game_state_t data)`

**Purpose**: Send game state data to paired ESP32

**Parameters**:
- `data`: Game state structure to transmit

**Returns**: 
- `true`: Data sent successfully
- `false`: Send failed (no connection, socket error)

**Usage Example**:
```cpp
game_state_t gameState;
gameState.foo = "player1";
gameState.bar = "move_complete";
gameState.score = 100;

if (sendData(gameState)) {
    printf("Game data sent successfully\n");
} else {
    printf("Failed to send game data\n");
}
```

**Network Details**:
- Uses UDP on port 3334
- Sends directly to partner IP address
- Non-blocking operation

---

### `void setReceiveData(void (*dataCallback)(game_state_t))`

**Purpose**: Continuously listen for incoming game data and trigger callback

**Parameters**:
- `dataCallback`: Function to call when data is received

**Behavior**:
- Runs in continuous loop until connection lost
- Validates data source (must be from paired partner)
- Calls callback function for each received message
- Automatically handles socket errors and retries

**Usage Example**:
```cpp
void handleGameData(game_state_t data) {
    printf("Received: %s, %s, score: %d\n", 
           data.foo.c_str(), data.bar.c_str(), data.score);
    
    // Process received game data
    updateGameState(data);
}

void logicMain() {
    main_data pairing = pair_esp();
    
    // Start background task for receiving data
    xTaskCreate([](void* param) {
        setReceiveData(&handleGameData);
    }, "receive_task", 4096, NULL, 5, NULL);
    
    // Main game loop continues...
    while(true) {
        // Game logic here
        delay(100);
    }
}
```

**Important**: 
- This function never returns (runs until connection lost)
- Should be called in a separate FreeRTOS task
- Only processes data from verified partner IP

## Internal Functions

### `wifi_init_sta()`
- Initializes WiFi in station mode
- Connects to "ChataULesa" network with password "ulesa529"
- Handles connection retries (max 5 attempts)
- Triggers device discovery on successful connection

### `initial_discovery_task()`
- Master behavior: sends 5 broadcast discovery messages
- Waits for responses with 2-second timeout
- On response: sets master mode and completes pairing
- On failure: switches to slave mode

### `listening_task()`
- Slave behavior: listens for discovery broadcasts
- Responds to master discovery messages
- Sets slave mode and completes pairing on discovery receipt

### `event_handler()`
- Handles WiFi events (connection, disconnection, IP assignment)
- Starts device discovery system when IP is obtained
- Manages connection retries and error states

## Configuration Constants

```cpp
#define WIFI_SSID "ChataULesa"      // Network name
#define WIFI_PASS "ulesa529"        // Network password  
#define WIFI_MAXIMUM_RETRY 5        // Max connection attempts
#define BROADCAST_PORT 3333         // Discovery port
```

## Global State Variables

```cpp
static esp_ip4_addr_t s_ip_addr;           // Local IP address
static esp_ip4_addr_t enemy_ip_addr;       // Partner IP address
static bool is_master;                     // Master/slave role
static bool connection_established;        // Pairing status
```

## Error Handling

The system includes comprehensive error handling for:

- **NVS Flash**: Automatic erase and re-initialization if needed
- **WiFi Connection**: Retry mechanism with failure detection
- **Socket Operations**: Error checking with appropriate cleanup
- **Network Timeouts**: Configurable timeouts for discovery operations
- **Partner Validation**: IP address verification for data security

## Threading Model

- **Main Thread**: Runs `pair_esp()` and main application logic
- **WiFi Event Task**: Handles WiFi events and starts discovery
- **Discovery Task**: Either `initial_discovery_task` or `listening_task`
- **Receive Task**: Runs `setReceiveData()` for continuous data reception

## Usage Patterns

### Basic Pairing and Communication
```cpp
void logicMain() {
    // 1. Pair the devices
    main_data pairing = pair_esp();
    
    // 2. Start receiving data in background
    xTaskCreate([](void* param) {
        setReceiveData(&handleGameData);
    }, "receive_task", 4096, NULL, 5, NULL);
    
    // 3. Main game loop
    while(true) {
        // Update game state
        game_state_t currentState = getGameState();
        
        // Send state to partner
        sendData(currentState);
        
        delay(100);
    }
}
```

### Error Recovery
```cpp
void logicMain() {
    try {
        main_data pairing = pair_esp();
        // ... setup communication ...
    } catch (const std::exception& e) {
        printf("Pairing failed: %s\n", e.what());
        // Handle pairing failure
    }
}
```

## Troubleshooting

### Common Issues

1. **Pairing Timeout**: 
   - Check WiFi network availability
   - Ensure both devices are on same network
   - Verify network allows UDP broadcast

2. **Data Not Received**:
   - Confirm `setReceiveData()` is running in separate task
   - Check firewall/network restrictions on port 3334
   - Verify callback function is properly defined

3. **Master/Slave Selection**:
   - First device to complete discovery becomes master
   - If both devices start simultaneously, timing determines roles
   - No manual role assignment needed

### Debug Information

The system provides extensive debug output:
- WiFi connection status
- Discovery attempt progress  
- Pairing success/failure messages
- Data transmission confirmations
- Error conditions and recovery attempts

Use serial monitor at 115200 baud to view debug output:
```bash
pio device monitor
```

## Dependencies

Required ESP-IDF components:
- `esp_wifi`: WiFi functionality
- `esp_event`: Event handling
- `esp_netif`: Network interface
- `nvs_flash`: Non-volatile storage
- `lwip`: TCP/IP stack
- `freertos`: Real-time OS

## Hardware Compatibility

Tested on Logic board:
- None :D, Thoughts and prayers 🙏