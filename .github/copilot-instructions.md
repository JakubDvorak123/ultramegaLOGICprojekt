# Logic Board ESP32 Project Instructions

This is an ESP32-based embedded project using PlatformIO and ESP-IDF framework, designed for custom Logic boards with displays and buttons.

## Project Architecture

- **Framework**: ESP-IDF with PlatformIO build system
- **Target**: ESP32/ESP32-S3 microcontrollers on custom Logic boards  
- **Language**: C++17 with exceptions enabled
- **Main Library**: [Logic_library](https://github.com/RoboticsBrno/Logic_library.git) provides `display`, `buttons`, `Rgb()` APIs

## Hardware Variants

The project supports multiple Logic board versions via environment-specific builds:
- `logic_1_1` (ESP32, default) - `LOGIC_VERSION_1_1` 
- `logic_1_2` (ESP32) - `LOGIC_VERSION_1_2`
- `logic_2_0` (ESP32-S3) - `LOGIC_VERSION_2_0`

Each variant has corresponding `sdkconfig.logic_X_X` files for hardware-specific configurations.

## Critical Development Workflows

### Building and Flashing
```bash
# Build for default environment (logic_1_1)
pio run

# Build for specific board version
pio run -e logic_1_2

# Flash firmware to device
pio run -t upload

# Upload filesystem (REQUIRED for /data files)
pio run -t uploadfs
```

**IMPORTANT**: Filesystem upload (`uploadfs`) is NOT automatic and must be run separately when `/data` contents change.

## File System & Data Management

- **SPIFFS partition**: 512KB at `0x380000` (see `partitions.csv`)
- **Data files**: Place in `/data/` directory, accessed via `/spiffs/` mount point
- **Image format**: 10x10 pixel raw RGB data files (`.data` extension)
  - Export from GIMP as "Raw image data"
  - 3 bytes per pixel (RGB), 300 bytes total per 10x10 image

### SPIFFS Usage Pattern
```cpp
// Standard SPIFFS initialization in logicMain()
esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true,
};
esp_vfs_spiffs_register(&conf);

// Read image data pixel-by-pixel
FILE* file = fopen("/spiffs/obrazec.data", "r");
uint8_t pixel[3];
for (int i = 0; i < display.width() * display.height(); ++i) {
    fread(pixel, 1, 3, file);
    display[i] = Rgb(pixel[0], pixel[1], pixel[2]);
}
```

## Logic Library APIs

- **Display**: `display.show(brightness)`, `display.clear()`, `display[index] = Rgb(r,g,b)`, `display.drawSquare(x,y,size,color)`
- **Buttons**: `buttons.readAll()` returns array with `R_Left`, `R_Right`, `L_Left`, `L_Right` indices
- **Entry Point**: Implement `void logicMain()` instead of standard `main()`

## Code Patterns

- Use `printf()` for debugging output (not `std::cout` in production code)
- File operations require explicit `fclose()` - memory is limited
- Prefer `uint8_t` for pixel/color data
- Comments often in Czech (project context)
- Button polling in main loop with `delay()` for debouncing

## Debugging

- Serial monitor: `pio device monitor` (115200 baud)
- Exception decoder enabled for stack trace analysis
- Debug build type configured by default
