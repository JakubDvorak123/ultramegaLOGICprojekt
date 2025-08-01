#ifndef libh
#define libh

#include<iostream>
#include<string>
#include<vector>

using namespace std;

#define Sx 10
#define Sy 10
#define malaNUM 3
#define velkaNUM 2
#define ponorkaNUM 1
#define kriznikNUM 1

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
    int lodbarva[Sx][Sy] = {0};
};

DIsplay dis;

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
    int kriznikLENy = 2;

    bool PONORKA[2][4]
    {
        {false, true, false, false},
        {true,  true, true,  true},
    };

    bool KRIZNIK[2][4]
    {
        {false, true, true, true},
        {true,  true, true, true},
    };

    void pridejLOD(int x, int y, int typ)
    {
        LodeProImplemntaci_NAdisplay.push_back(typ);
        lodey.push_back(y);
        lodex.push_back(x);
    }
    int GETlodeNUM( void ) const { return LodeProImplemntaci_NAdisplay.size(); }
    void ResetLODE()
    {
        LodeProImplemntaci_NAdisplay.clear();
        lodex.clear();
        lodey.clear();
    }
    bool isValidPlacement(int x, int y, int typ) const {
        if (typ == lod::mala) {
            // Check 2x1 ship
            for (int i = 0; i < malaLEN; i++) {
                if (x + i >= 0 && x + i < Sx && dis.policka[x + i][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (x + i + 1 >= 0 && x + i + 1 < Sx && dis.policka[x + i + 1][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (x + i - 1 >= 0 && x + i - 1 < Sx && dis.policka[x + i - 1][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (y + 1 >= 0 && y + 1 < Sy && dis.policka[x + i][y + 1] == DIsplay::stav::plna) {
                    return false;
                }
                if (y - 1 >= 0 && y - 1 < Sy && dis.policka[x + i][y - 1] == DIsplay::stav::plna) {
                    return false;
                }
            }
        } else if (typ == lod::velka) {
            // Check 4x1 ship
            for (int i = 0; i < velkaLEN; i++) {
                if (x + i >= 0 && x + i < Sx && dis.policka[x + i][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (x + i + 1 >= 0 && x + i + 1 < Sx && dis.policka[x + i + 1][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (x + i - 1 >= 0 && x + i - 1 < Sx && dis.policka[x + i - 1][y] == DIsplay::stav::plna) {
                    return false;
                }
                if (y + 1 >= 0 && y + 1 < Sy && dis.policka[x + i][y + 1] == DIsplay::stav::plna) {
                    return false;
                }
                if (y - 1 >= 0 && y - 1 < Sy && dis.policka[x + i][y - 1] == DIsplay::stav::plna) {
                    return false;
                }
            }
        } else if (typ == lod::ponorka) {
            // Check 4x2 submarine
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < ponorkaLENx; dx++) {
                    if (PONORKA[dy][dx] && dis.policka[x + dx][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (PONORKA[dy][dx] && dis.policka[x + dx + 1][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (PONORKA[dy][dx] && dis.policka[x + dx - 1][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (PONORKA[dy][dx] && dis.policka[x + dx][y + dy + 1] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (PONORKA[dy][dx] && dis.policka[x + dx][y + dy - 1] == DIsplay::stav::plna) {
                        return false;
                    }
                }
            }
        } else if (typ == lod::kriznik) {
            // Check 4x2 cruiser
            for (int dy = 0; dy < 2; dy++) {
                for (int dx = 0; dx < kriznikLENx; dx++) {
                    if (KRIZNIK[dy][dx] && dis.policka[x + dx][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (KRIZNIK[dy][dx] && dis.policka[x + dx + 1][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (KRIZNIK[dy][dx] && dis.policka[x + dx - 1][y + dy] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (KRIZNIK[dy][dx] && dis.policka[x + dx][y + dy + 1] == DIsplay::stav::plna) {
                        return false;
                    }
                    if (KRIZNIK[dy][dx] && dis.policka[x + dx][y + dy - 1] == DIsplay::stav::plna) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool assignLOD(int ID)
    {
        for(int y = 0; y < Sy; y++)
        {
            for(int x = 0; x < Sx; x++)
            {
                if(LodeProImplemntaci_NAdisplay[ID] == lod::mala)
                {
                    for(int i = 0; i < malaLEN; i++)
                    {   
                        if(x == lodex[ID] + i && y == lodey[ID])
                        {
                            dis.policka[x][y] = DIsplay::stav::plna;
                            dis.lodbarva[x][y] = 1;
                        }
                    }
                }
                if(LodeProImplemntaci_NAdisplay[ID] == lod::velka)
                {
                    for(int i = 0; i < velkaLEN; i++)
                    {   
                        if(x == lodex[ID] + i && y == lodey[ID])
                        {
                            dis.policka[x][y] = DIsplay::stav::plna;
                            dis.lodbarva[x][y] = 2;
                        }
                    }
                }
                if(LodeProImplemntaci_NAdisplay[ID] == lod::ponorka)
                {
                    for(int j = 0; j < ponorkaLENy; j++)
                    {
                        for(int i = 0; i < ponorkaLENx; i++)
                        {   
                            if(x == lodex[ID] + i && y == lodey[ID] + j)
                            {
                                if(PONORKA[j][i] == true)
                                {
                                    dis.policka[x][y] = DIsplay::stav::plna;
                                    dis.lodbarva[x][y] = 3;
                                }
                            }
                        }
                    }
                }
                if(LodeProImplemntaci_NAdisplay[ID] == lod::kriznik)
                {
                    for(int j = 0; j < kriznikLENy; j++)
                    {
                        for(int i = 0; i < kriznikLENx; i++)
                        {   
                            if(x == lodex[ID] + i && y == lodey[ID] + j)
                            {
                                if(KRIZNIK[j][i] == true)
                                {
                                    dis.policka[x][y] = DIsplay::stav::plna;
                                    dis.lodbarva[x][y] = 4;
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    void Render();

private:
    vector<int> LodeProImplemntaci_NAdisplay;
    vector<int> lodex, lodey;
};

#endif //libh