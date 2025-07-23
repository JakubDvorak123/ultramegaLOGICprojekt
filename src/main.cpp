#include "Logic.hpp"
#include <iostream>

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

    DIsplay()
    {

    }

    typedef struct
    {
        enum stavENUM : int
        {
            prazdna, 
            plna,
            zasazena, 
        };
    } stav; 


    
    int policka[Sx][Sy] = {0};
};

class LOde
{
public:
    struct lod
    {
        enum typ
        {
            mala, 
            velka, 
            ponorka,
            kriznik
        };
        bool zasah = false;
        int rotation = 0;
    };

    int malaLEN = 2;
    int velkaLEN = 4;
    int ponorkaLENx = 4;
    int ponorkaLENy = 2;
    int kriznikLENx = 4;
    int kriznikLENy = 4;

    bool MALA[2]
    {
        true, true,
    };

    bool VELKA[4]
    {
        true, true, true, true
    };

    bool PONORKA[4][2]
    {
        false, true, false, false,
        true,  true, true,  true,
    };

    bool KRIZNIK[4][2]
    {
        false, true, true, true,
        true,  true, true, true,
    };


};


void logicMain()
 {
    DIsplay dis;

    int lodeNUM = 3;
    LOde::lod NaselodeTYPY[lodeNUM];

    for(int i = 0 ; i < lodeNUM; i++)
    {

    }

    for(int y = 0; y < Sy; y ++)
    {
        for(int x = 0; x < Sx; x++)
        {
            //if(x == 0 || y == 0 || x == 9 || y == 9)
            //{
                dis.policka[x][y] = DIsplay::stav::plna;
            //}
        }
    }

    while(true)
    {
        for(int y = 0; y < Sy; y++)
        {
            for(int x = 0; x < Sx; x++)
            {
                if(dis.policka[x][y] == DIsplay::stav::plna)
                {
                    display.at(x, y) = Rgb(100, 25, 100);
                }
            }
        }

        display.show(10);
        delay(100);
        display.clear();

    }

   // display.clear();
    
    std::cout << "Hello world!\n";
}