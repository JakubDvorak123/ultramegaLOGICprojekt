#include "Logic.hpp"
#include <iostream>

// Potřebné include pro práci se soubory
#include "esp_spiffs.h"
#include <cstdio>




// void logicMain() {

    // Ve složce /data v tomto projektu jsou soubory s obrázky.
// Obrázky musí být 10x10 pixelů v GIMPU, exportovaná jako "surová data",
// přípona .data.
//
// /data je potřeba flashnout do logika pomocí příkazu "pio run -t uploadfs",
// !!! NEFLASHUJE SE SAMO !!!
// Ten příkaz napište do terminálu, který otevřete kliknutím na tlačítko "terminál"
// na dolním baru, je napravo od flashovací šipky.

//     // Konfigurace datového oddílu
//     esp_vfs_spiffs_conf_t conf = {
//         .base_path = "/spiffs",
//         .partition_label = NULL,
//         .max_files = 5,
//         .format_if_mount_failed = true,
//     };

//     // Připojení datového oddílu na /spiffs
//     esp_err_t ret = esp_vfs_spiffs_register(&conf);
//     if (ret != ESP_OK) {
//         if (ret == ESP_FAIL) {
//             printf("Failed to mount or format filesystem\n");
//         } else if (ret == ESP_ERR_NOT_FOUND) {
//             printf("Failed to find SPIFFS partition\n");
//         } else {
//             printf("Failed to initialize SPIFFS (%d)\n", ret);
//         }
//         return;
//     }

//     // Otevření souboru
//     FILE* soubor = fopen("/spiffs/obrazec.data", "r");
//     if (!soubor) {
//         printf("Failed to open file: %s\n", strerror(errno));
//         return;
//     }

//     // Načtení dat ze souboru, pixel po pixelu.
//     uint8_t pixel[3];
//     for (int i = 0; i < display.width() * display.height(); ++i) {
//         size_t res = fread(pixel, 1, 3, soubor);
//         if (res != 3) {
//             printf("Nepodarilo se precist 3 byty %d\n", res);
//             break;
//         }
//         // Nastavení do displeje
//         display[i] = Rgb(pixel[0], pixel[1], pixel[2]);
//     }

//     // Zavření souboru - nezapomeňte na to!
//     fclose(soubor);

//     display.show(20);
// }

using namespace std;
#define Sx 10
#define Sy 10

typedef unsigned short int USint;

struct vec2
{
    USint x, y;
};

class DIsplay
{
public:

    typedef struct
    {
        enum stavENUM
        {
            prazdna, 
            plna,
            zasazena, 
        };
    } stav; 

    stav policka[Sx][Sy];
};



void logicMain()
 {

    //while(true)
    //{
 
        display.at(0, 0) = Rgb(255, 0, 0);
        display.show(30);
        delay(3000);
        display.clear();

    //}

   // display.clear();
    
    std::cout << "Hello world!\n";
}