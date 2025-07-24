#include "Logic.hpp"
#include "lib.h"

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
    //int lodeNUM = 3;
    LOde lode;
    THEhra h;    
    /**
     * propojovani, po propojeni THEhra.LocalniStav = umistovani, jinak cekani;
     */
    while(h.LocalniStav == THEhra::cekani)
    {
        h.LocalniStav = THEhra::umistovani;
    }
    
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
                lode.pridejLOD(Vx, Vy, LOde::lod::mala);
                lode.assignLOD(umistovani - 1);
                umistovani++;
            }
            else if(umistovani <= malaNUM + velkaNUM)
            {
                if(Vx + lode.velkaLEN < Sx + 1)
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::velka);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            }
            else if(umistovani <= malaNUM + velkaNUM + ponorkaNUM)
            {
                if(Vx + lode.ponorkaLENx < Sx + 1 && Vy + lode.ponorkaLENy < Sy + 1)
                {
                    lode.pridejLOD(Vx, Vy, LOde::lod::ponorka);
                    lode.assignLOD(umistovani - 1);
                    umistovani++;
                }
            
            }
            else if(umistovani <= malaNUM + velkaNUM + ponorkaNUM + kriznikNUM)
            {
                if(Vx + lode.kriznikLENx < Sx + 1 && Vy + lode.ponorkaLENy < Sy + 1)
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


    while(h.LocalniStav == THEhra::hra)
    {

        lode.Render();

        display.show(10);
        delay(100);
        display.clear();

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
                if(dis.lodbarva[x][y] == 1)
                {
                    display.at(x, y) = Rgb(100, 0, 0);
                }
                if(dis.lodbarva[x][y] == 2)
                {
                    display.at(x, y) = Rgb(0, 100, 0);
                }
                if(dis.lodbarva[x][y] == 3)
                {
                    display.at(x, y) = Rgb(0, 0, 100);
                }
                if(dis.lodbarva[x][y] == 4)
                {
                    display.at(x, y) = Rgb(100, 100, 0);
                }
                if(dis.lodbarva[x][y] == 5)
                {
                    display.at(x, y) = Rgb(100, 20, 75);
                }
            }
        }
    }
}