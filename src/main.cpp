#include "Logic.hpp"
#include "lib.h"
#include "types.h"
#include "networking.hpp"
#include <cstring>

// Global game state variables
static bool my_turn = false;
static bool game_active = false;
static bool opponent_ready = false;
static int enemy_grid[10][10] = {0}; // 0=unknown, 1=miss, 2=hit
static int my_hits = 0;
static int enemy_hits = 0;
static game_state_t incoming_shot = {}; // Store incoming shot
static bool shot_received = false;

// Function to check if a shot hits our ships
bool checkHit(int x, int y, LOde& lode) {
    if (x < 0 || x >= Sx || y < 0 || y >= Sy) return false;
    return dis.policka[x][y] == DIsplay::stav::plna;
}

void handleReceiveData(game_state_t data){
    printf("Received: action=%s, x=%d, y=%d, hit=%d\n", 
           data.action, data.target_x, data.target_y, data.is_hit);
    
    if (strcmp(data.action, "ready") == 0) {
        opponent_ready = true;
        printf("Opponent is ready!\n");
    }
    else if (strcmp(data.action, "shoot") == 0) {
        // Store the incoming shot to be processed in main loop
        incoming_shot = data;
        shot_received = true;
        printf("Opponent shot at (%d,%d)\n", data.target_x, data.target_y);
    }
    else if (strcmp(data.action, "hit") == 0 || strcmp(data.action, "miss") == 0) {
        // Response to our shot
        enemy_grid[data.target_x][data.target_y] = data.is_hit ? 2 : 1;
        if (data.is_hit) {
            my_hits++;
            printf("HIT! Enemy ship damaged!\n");
        } else {
            printf("Miss!\n");
        }
        my_turn = true; // Continue our turn if we hit, or opponent's turn if we missed
    }
    else if (strcmp(data.action, "game_over") == 0) {
        printf("Game Over! %s\n", data.message);
        game_active = false;
    }
};

struct THEhra
{
    enum Hrastav
    {
        umistovani, 
        hra, 
        konec, 
        
        cekani,
        prijeti, 
        neprijeti,
    };  
    int LocalniStav = cekani;
    };

void logicMain()
{
    // Temporarily disabled networking for single-board testing
    main_data maindata;
    maindata = pair_esp();
    xTaskCreate([](void* param) {
            setReceiveData(&handleReceiveData);
    }, "receive_task", 4096, NULL, 5, NULL);

    LOde lode;
    THEhra h;    
    // Skip waiting state and go directly to ship placement
    h.LocalniStav = THEhra::umistovani;
    
    int Vx = 0, Vy = 0;
    int umistovani = 1;
    bool enter = false;
    while(h.LocalniStav == THEhra::umistovani)
    {
        if(buttons.read(R_Right) && Vx+1 < Sx) { Vx++; }
        if(buttons.read(R_Left) && Vx > 0){ Vx--; }
        if(buttons.read(R_Up) && Vy > 0) { Vy--; }
        if(buttons.read(R_Down) && Vy+1 < Sy) { Vy++; }
        if(buttons.read(R_Enter) == true && enter == false) 
        {
            if(umistovani <= malaNUM)
            {
                if (Vx + lode.malaLEN <= Sx && lode.isValidPlacement(Vx, Vy, LOde::lod::mala))
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::mala);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            }
            else if(umistovani <= malaNUM + velkaNUM)
            {
                if(Vx + lode.velkaLEN < Sx + 1 && lode.isValidPlacement(Vx, Vy, LOde::lod::velka))
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::velka);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            }
            else if(umistovani <= malaNUM + velkaNUM + ponorkaNUM)
            {
                if(Vx + lode.ponorkaLENx < Sx + 1 && Vy + lode.ponorkaLENy < Sy + 1 &&
                   lode.isValidPlacement(Vx, Vy, LOde::lod::ponorka))
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::ponorka);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            
            }
            else if(umistovani <= malaNUM + velkaNUM + ponorkaNUM + kriznikNUM)
            {
                if(Vx + lode.kriznikLENx < Sx + 1 && Vy + lode.kriznikLENy < Sy + 1 &&
                   lode.isValidPlacement(Vx, Vy, LOde::lod::kriznik))
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::kriznik);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            }
            else 
            {
                h.LocalniStav = THEhra::hra;
            }
            enter = true;
        }
        if(buttons.read(R_Enter) == false && enter == true) {enter = false;}
    
        lode.Render();
        display.at(Vx, Vy) = Rgb(100, 20, 75);
        display.show(10);
        delay(100);
        display.clear();
    }

    std::cout << "lode num>>" << lode.GETlodeNUM() << endl;

    // Send ready signal to opponent
    game_state_t readySignal;
    strcpy(readySignal.action, "ready");
    readySignal.target_x = 0;
    readySignal.target_y = 0;
    readySignal.score = 0;
    readySignal.is_hit = false;
    readySignal.game_over = false;
    strcpy(readySignal.message, "Player ready");
    sendData(readySignal);
    
    // Wait for both players to be ready
    printf("Waiting for opponent to be ready...\n");
    while (!opponent_ready) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    printf("Both players ready! Starting game...\n");
    game_active = true;
    my_turn = maindata.is_master; // Master goes first
    
    int cursor_x = 0, cursor_y = 0;
    bool shoot_pressed = false;

    while(h.LocalniStav == THEhra::hra && game_active)
    {
        // Handle incoming shots from opponent
        if (shot_received) {
            bool hit = checkHit(incoming_shot.target_x, incoming_shot.target_y, lode);
            
            // Send response
            game_state_t response;
            strcpy(response.action, hit ? "hit" : "miss");
            response.target_x = incoming_shot.target_x;
            response.target_y = incoming_shot.target_y;
            response.score = enemy_hits;
            response.is_hit = hit;
            response.game_over = false;
            strcpy(response.message, hit ? "Hit!" : "Miss!");
            sendData(response);
            
            if (hit) {
                enemy_hits++;
                printf("Enemy hit our ship at (%d,%d)!\n", incoming_shot.target_x, incoming_shot.target_y);
                // Mark as hit on our board
                dis.policka[incoming_shot.target_x][incoming_shot.target_y] = DIsplay::stav::zasazena;
            } else {
                printf("Enemy missed at (%d,%d)\n", incoming_shot.target_x, incoming_shot.target_y);
            }
            
            my_turn = true; // It's now our turn
            shot_received = false;
        }
        
        // Clear display
        display.clear();
        
        // Show enemy grid if it's our turn
        if (my_turn) {
            // Show cursor for targeting
            if (cursor_x >= 0 && cursor_x < Sx && cursor_y >= 0 && cursor_y < Sy) {
                display.at(cursor_x, cursor_y) = Rgb(255, 255, 255); // White cursor
            }
            
            // Handle movement controls for targeting
            if(buttons.read(R_Right) && cursor_x + 1 < Sx) { 
                cursor_x++; 
                vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
            }
            if(buttons.read(R_Left) && cursor_x > 0) { 
                cursor_x--; 
                vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
            }
            if(buttons.read(R_Up) && cursor_y > 0) { 
                cursor_y--; 
                vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
            }
            if(buttons.read(R_Down) && cursor_y + 1 < Sy) { 
                cursor_y++; 
                vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
            }
            
            // Handle shooting
            if(buttons.read(R_Enter) && !shoot_pressed) {
                if (enemy_grid[cursor_x][cursor_y] == 0) { // Haven't shot here yet
                    // Send shot to opponent
                    game_state_t shot;
                    strcpy(shot.action, "shoot");
                    shot.target_x = cursor_x;
                    shot.target_y = cursor_y;
                    shot.score = my_hits;
                    shot.is_hit = false;
                    shot.game_over = false;
                    strcpy(shot.message, "Shot fired");
                    sendData(shot);
                    
                    printf("Shot fired at (%d,%d)\n", cursor_x, cursor_y);
                    my_turn = false; // Wait for response
                }
                shoot_pressed = true;
            }
            if(!buttons.read(R_Enter)) {
                shoot_pressed = false;
            }
            
            if(buttons.read(L_Enter)) {
                // Render our ships
                lode.Render();
            }
            else {
                // Display enemy grid status
                for(int y = 0; y < Sy; y++) {
                    for(int x = 0; x < Sx; x++) {
                        if (enemy_grid[x][y] == 1) {
                            display.at(x, y) = Rgb(50, 50, 50); // Miss - gray
                        } else if (enemy_grid[x][y] == 2) {
                            display.at(x, y) = Rgb(255, 0, 0); // Hit - red
                        }
                    }
                }
            }
        } else {
            // Not our turn - show waiting message by blinking display
            static int blink_counter = 0;
            blink_counter++;
            if ((blink_counter / 10) % 2 == 0) {
                display.at(4, 4) = Rgb(0, 0, 255); // Blue waiting indicator
                display.at(5, 4) = Rgb(0, 0, 255);
                display.at(4, 5) = Rgb(0, 0, 255);
                display.at(5, 5) = Rgb(0, 0, 255);
            }
        }

        display.show(10);
        delay(50);
        
        // Check for game over conditions
        if (my_hits >= 10) { // Assuming 10 hits to win
            game_state_t gameOver;
            strcpy(gameOver.action, "game_over");
            gameOver.target_x = 0;
            gameOver.target_y = 0;
            gameOver.score = my_hits;
            gameOver.is_hit = false;
            gameOver.game_over = true;
            strcpy(gameOver.message, "You lose!");
            sendData(gameOver);
            
            printf("You win! Game over.\n");
            h.LocalniStav = THEhra::konec;
        }
        if (enemy_hits >= 10) {
            printf("You lose! Game over.\n");
            h.LocalniStav = THEhra::konec;
        }
    }

    // Game over display
    while(h.LocalniStav == THEhra::konec) {
        display.clear();
        // Show game over pattern
        for(int i = 0; i < 10; i++) {
            display.at(i, i) = Rgb(255, 255, 0); // Yellow diagonal
            display.at(9-i, i) = Rgb(255, 255, 0);
        }
        display.show(10);
        delay(500);
        
        display.clear();
        display.show(10);
        delay(500);
    }

   // display.clear();
    
    std::cout << "Hello world!\n";
}

//RENDER pristup
void LOde::Render()
{
    for(int y = 0; y < Sy; y++)
    {
        for(int x = 0; x < Sx; x++)
        {
            if(dis.policka[x][y] == DIsplay::stav::plna)
            {
                // Normal ship colors
                if(dis.lodbarva[x][y] == 1)
                {
                    display.at(x, y) = Rgb(100, 0, 0);
                }
                else if(dis.lodbarva[x][y] == 2)
                {
                    display.at(x, y) = Rgb(0, 100, 0);
                }
                else if(dis.lodbarva[x][y] == 3)
                {
                    display.at(x, y) = Rgb(0, 0, 100);
                }
                else if(dis.lodbarva[x][y] == 4)
                {
                    display.at(x, y) = Rgb(100, 100, 0);
                }
                else if(dis.lodbarva[x][y] == 5)
                {
                    display.at(x, y) = Rgb(100, 20, 75);
                }
            }
            else if(dis.policka[x][y] == DIsplay::stav::zasazena)
            {
                // Hit ship - bright red
                display.at(x, y) = Rgb(255, 0, 0);
            }
        }
    }
}