/* s_gen.c
 *
 * Micropolis, Unix Version.  This game was released for the Unix platform
 * in or about 1990 and has been modified for inclusion in the One Laptop
 * Per Child program.  Copyright (C) 1989 - 2007 Electronic Arts Inc.  If
 * you need assistance with this program, you may contact:
 *   http://wiki.laptop.org/go/Micropolis  or email  micropolis@laptop.org.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.  You should have received a
 * copy of the GNU General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 * 
 *             ADDITIONAL TERMS per GNU GPL Section 7
 * 
 * No trademark or publicity rights are granted.  This license does NOT
 * give you any right, title or interest in the trademark SimCity or any
 * other Electronic Arts trademark.  You may not distribute any
 * modification of this program using the trademark SimCity or claim any
 * affliation or association with Electronic Arts Inc. or its employees.
 * 
 * Any propagation or conveyance of this program must include this
 * copyright notice and these terms.
 * 
 * If you convey this program (or any modifications of it) and assume
 * contractual liability for the program to recipients of it, you agree
 * to indemnify Electronic Arts for any liability that those contractual
 * assumptions impose on Electronic Arts.
 * 
 * You may not misrepresent the origins of this program; modified
 * versions of the program must be marked as such and not identified as
 * the original program.
 * 
 * This disclaimer supplements the one included in the General Public
 * License.  TO THE FULLEST EXTENT PERMISSIBLE UNDER APPLICABLE LAW, THIS
 * PROGRAM IS PROVIDED TO YOU "AS IS," WITH ALL FAULTS, WITHOUT WARRANTY
 * OF ANY KIND, AND YOUR USE IS AT YOUR SOLE RISK.  THE ENTIRE RISK OF
 * SATISFACTORY QUALITY AND PERFORMANCE RESIDES WITH YOU.  ELECTRONIC ARTS
 * DISCLAIMS ANY AND ALL EXPRESS, IMPLIED OR STATUTORY WARRANTIES,
 * INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY, SATISFACTORY QUALITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS, AND WARRANTIES (IF ANY) ARISING FROM A COURSE OF DEALING,
 * USAGE, OR TRADE PRACTICE.  ELECTRONIC ARTS DOES NOT WARRANT AGAINST
 * INTERFERENCE WITH YOUR ENJOYMENT OF THE PROGRAM; THAT THE PROGRAM WILL
 * MEET YOUR REQUIREMENTS; THAT OPERATION OF THE PROGRAM WILL BE
 * UNINTERRUPTED OR ERROR-FREE, OR THAT THE PROGRAM WILL BE COMPATIBLE
 * WITH THIRD PARTY SOFTWARE OR THAT ANY ERRORS IN THE PROGRAM WILL BE
 * CORRECTED.  NO ORAL OR WRITTEN ADVICE PROVIDED BY ELECTRONIC ARTS OR
 * ANY AUTHORIZED REPRESENTATIVE SHALL CREATE A WARRANTY.  SOME
 * JURISDICTIONS DO NOT ALLOW THE EXCLUSION OF OR LIMITATIONS ON IMPLIED
 * WARRANTIES OR THE LIMITATIONS ON THE APPLICABLE STATUTORY RIGHTS OF A
 * CONSUMER, SO SOME OR ALL OF THE ABOVE EXCLUSIONS AND LIMITATIONS MAY
 * NOT APPLY TO YOU.
 */
#include "main.h"

#include "s_alloc.h"
#include "s_init.h"
#include "s_sim.h"

#include "w_tk.h"
#include "w_update.h"
#include "w_util.h"


#include <algorithm>
#include <iostream>
#include <limits>

/* Generate Map */


#define WATER_LOW	RIVER /* 2 */
#define WATER_HIGH	LASTRIVEDGE /* 20 */
#define WOODS_LOW	TREEBASE /* 21 */
#define WOODS_HIGH	UNUSED_TRASH2 /* 39 */


constexpr auto RADIUS = 18;


int XStart, YStart, MapX, MapY;
int Dir, LastDir;
int TreeLevel = -1;		/* level for tree creation */
int LakeLevel = -1;		/* level for lake creation */
int CurveLevel = -1;		/* level for river curviness */
int CreateIsland = -1;		/* -1 => 10%, 0 => never, 1 => always */


int ERand(int limit)
{
  int x, z;

  z = RandomRange(0, limit);
  x = RandomRange(0, limit);
  
  return std::min(x, z);
}


bool IsTree(int cell)
{
    return (((cell & LOMASK) >= WOODS_LOW) && ((cell & LOMASK) <= WOODS_HIGH));
}


void ClearMap()
{
    for (int x = 0; x < SimWidth; x++)
    {
        for (int y = 0; y < SimHeight; y++)
        {
            Map[x][y] = DIRT;
        }
    }
}


void ClearUnnatural()
{
    for (int x = 0; x < SimWidth; x++)
    {
        for (int y = 0; y < SimHeight; y++)
        {
            if (Map[x][y] > WOODS)
            {
                Map[x][y] = DIRT;
            }
        }
    }
}


void MoveMap(int dir)
{
    static int DirTab[2][8] =
    {
        { 0, 1, 1, 1, 0, -1, -1, -1},
        {-1,-1, 0, 1, 1,  1,  0, -1}
    };

    MapX += DirTab[0][dir & 7];
    MapY += DirTab[1][dir & 7];
}


void PutOnMap(int Mchar, int Xoff, int Yoff)
{
    if (Mchar == 0)
    {
        return;
    }

    int Xloc = MapX + Xoff;
    int Yloc = MapY + Yoff;

    if (!TestBounds(Xloc, Yloc, SimWidth, SimHeight))
    {
        return;
    }

    int temp = 0;
    if (temp = Map[Xloc][Yloc])
    {
        temp = temp & LOMASK;
        if (temp == RIVER)
        {
            if (Mchar != CHANNEL)
            {
                return;
            }
        }
        if (temp == CHANNEL)
        {
            return;
        }
    }

    Map[Xloc][Yloc] = Mchar;
}


void SmoothTrees()
{
    static int DX[4] = { -1, 0, 1, 0 };
    static int DY[4] = { 0, 1, 0,-1 };

    static int TEdTab[16] =
    {
        0, 0, 0, 34,
        0, 0, 36, 35,
        0, 32, 0, 33,
        30, 31, 29, 37
    };

    for (int MapX = 0; MapX < SimWidth; MapX++)
    {
        for (int MapY = 0; MapY < SimHeight; MapY++)
        {
            if (IsTree(Map[MapX][MapY]))
            {
                int bitindex = 0;
                for (int z = 0; z < 4; z++)
                {
                    bitindex = bitindex << 1;

                    int Xtem = MapX + DX[z];
                    int Ytem = MapY + DY[z];

                    if (TestBounds(Xtem, Ytem, SimWidth, SimHeight) && IsTree(Map[Xtem][Ytem]))
                    {
                        bitindex++;
                    }
                }

                int temp = TEdTab[bitindex & 15];

                if (temp)
                {
                    if (temp != WOODS)
                    {
                        if ((MapX + MapY) & 1)
                        {
                            temp = temp - 8;
                        }
                    }
                    Map[MapX][MapY] = temp + BLBNBIT;
                }
                else
                {
                    Map[MapX][MapY] = temp;
                }
            }
        }
    }
}


void SmoothRiver()
{
    static int DX[4] = { -1, 0, 1, 0 };
    static int DY[4] = { 0, 1, 0,-1 };

    static int REdTab[16] =
    {
        13 + BULLBIT, 13 + BULLBIT, 17 + BULLBIT, 15 + BULLBIT,
        5 + BULLBIT, 2, 19 + BULLBIT, 17 + BULLBIT,
        9 + BULLBIT, 11 + BULLBIT, 2, 13 + BULLBIT,
        7 + BULLBIT, 9 + BULLBIT, 5 + BULLBIT, 2
    };

    for (int MapX = 0; MapX < SimWidth; MapX++)
    {
        for (int MapY = 0; MapY < SimHeight; MapY++)
        {
            if (Map[MapX][MapY] == REDGE)
            {
                int bitindex = 0;

                for (int z = 0; z < 4; z++)
                {
                    bitindex = bitindex << 1;
                    int Xtem = MapX + DX[z];
                    int Ytem = MapY + DY[z];
                    if (TestBounds(Xtem, Ytem, SimWidth, SimHeight) &&
                        ((Map[Xtem][Ytem] & LOMASK) != DIRT) &&
                        (((Map[Xtem][Ytem] & LOMASK) < WOODS_LOW) ||
                            ((Map[Xtem][Ytem] & LOMASK) > WOODS_HIGH)))
                    {
                        bitindex++;
                    }

                }

                int temp = REdTab[bitindex & 15];

                if ((temp != RIVER) && (RandomRange(0, 1)))
                {
                    temp++;
                }

                Map[MapX][MapY] = temp;
            }
        }
    }
}



void TreeSplash(int xloc, int yloc)
{
    int dis = TreeLevel < 0 ? dis = RandomRange(0, 150) + 50 : RandomRange(0, 100 + (TreeLevel * 2)) + 50;

    MapX = xloc;
    MapY = yloc;

    for (int z = 0; z < dis; z++)
    {
        int dir = RandomRange(0, 7);

        MoveMap(dir);

        if (!(TestBounds(MapX, MapY, SimWidth, SimHeight)))
        {
            return;
        }

        if ((Map[MapX][MapY] & LOMASK) == DIRT)
        {
            Map[MapX][MapY] = WOODS + BLBNBIT;
        }
    }
}


void DoTrees()
{
    int Amount, x, xloc, yloc;

    if (TreeLevel < 0)
    {
        Amount = RandomRange(0, 100) + 50;
    }
    else
    {
        Amount = TreeLevel + 3;
    }

    for (x = 0; x < Amount; x++)
    {
        xloc = RandomRange(0, SimWidth - 1);
        yloc = RandomRange(0, SimHeight - 1);
        TreeSplash(xloc, yloc);
    }

    SmoothTrees();
    SmoothTrees();
}


void BRivPlop()
{
    static int BRMatrix[9][9] =
    {
        { 0, 0, 0, 3, 3, 3, 0, 0, 0 },
        { 0, 0, 3, 2, 2, 2, 3, 0, 0 },
        { 0, 3, 2, 2, 2, 2, 2, 3, 0 },
        { 3, 2, 2, 2, 2, 2, 2, 2, 3 },
        { 3, 2, 2, 2, 4, 2, 2, 2, 3 },
        { 3, 2, 2, 2, 2, 2, 2, 2, 3 },
        { 0, 3, 2, 2, 2, 2, 2, 3, 0 },
        { 0, 0, 3, 2, 2, 2, 3, 0, 0 },
        { 0, 0, 0, 3, 3, 3, 0, 0, 0 }
    };

    for (int x = 0; x < 9; x++)
    {
        for (int y = 0; y < 9; y++)
        {
            PutOnMap(BRMatrix[y][x], x, y);
        }
    }
}


void SRivPlop(void)
{
    static int SRMatrix[6][6] =
    {
        { 0, 0, 3, 3, 0, 0 },
        { 0, 3, 2, 2, 3, 0 },
        { 3, 2, 2, 2, 2, 3 },
        { 3, 2, 2, 2, 2, 3 },
        { 0, 3, 2, 2, 3, 0 },
        { 0, 0, 3, 3, 0, 0 }
    };

    for (int x = 0; x < 6; x++)
    {
        for (int y = 0; y < 6; y++)
        {
            PutOnMap(SRMatrix[y][x], x, y);
        }
    }
}


void DoBRiv()
{
    int r1, r2;

    if (CurveLevel < 0)
    {
        r1 = 100;
        r2 = 200;
    }
    else
    {
        r1 = CurveLevel + 10;
        r2 = CurveLevel + 100;
    }

    while (TestBounds(MapX + 4, MapY + 4, SimWidth, SimHeight))
    {
        BRivPlop();
        if (RandomRange(0, r1) < 10)
        {
            Dir = LastDir;
        }
        else
        {
            if (RandomRange(0, r2) > 90) Dir++;
            if (RandomRange(0, r2) > 90) Dir--;
        }
        MoveMap(Dir);
    }
}


void DoSRiv()
{
    int r1, r2;

    if (CurveLevel < 0)
    {
        r1 = 100;
        r2 = 200;
    }
    else
    {
        r1 = CurveLevel + 10;
        r2 = CurveLevel + 100;
    }

    while (TestBounds(MapX + 3, MapY + 3, SimWidth, SimHeight))
    {
        SRivPlop();
        if (RandomRange(0, r1) < 10)
        {
            Dir = LastDir;
        }
        else
        {
            if (RandomRange(0, r2) > 90) Dir++;
            if (RandomRange(0, r2) > 90) Dir--;
        }
        MoveMap(Dir);
    }
}


void DoRivers()
{
    LastDir = RandomRange(0, 3);
    Dir = LastDir;   
    DoBRiv();
    
    MapX = XStart;
    MapY = YStart;
    LastDir = LastDir ^ 4;
    Dir = LastDir;
    DoBRiv();

    MapX = XStart;
    MapY = YStart;
    LastDir = RandomRange(0, 3);
    DoSRiv();
}


void MakeNakedIsland()
{
int x, y;

  for (x = 0; x < SimWidth; x++)
    for (y = 0; y < SimHeight; y++)
      Map[x][y] = RIVER;
  for (x = 5; x < SimWidth - 5; x++)
    for (y = 5; y < SimHeight - 5; y++)
      Map[x][y] = DIRT;
  for (x = 0; x < SimWidth - 5; x += 2) {
    MapX = x ;
    MapY = ERand(RADIUS);
    BRivPlop();
    MapY = (SimHeight - 10) - ERand(RADIUS);
    BRivPlop();
    MapY = 0;
    SRivPlop();
    MapY = (SimHeight - 6);
    SRivPlop();
  }
  for (y = 0; y < SimHeight - 5; y += 2) {
    MapY = y ;
    MapX = ERand(RADIUS);
    BRivPlop();
    MapX = (SimWidth - 10) - ERand(RADIUS);
    BRivPlop();
    MapX = 0;
    SRivPlop();
    MapX = (SimWidth - 6);
    SRivPlop();
  }
}


void MakeIsland()
{
    MakeNakedIsland();
    SmoothRiver();
    DoTrees();
}


void MakeLakes()
{
    int Lim1 = 0;
    int Lim2 = 0;

    if (LakeLevel < 0)
    {
        Lim1 = RandomRange(0, 10);
    }
    else
    {
        Lim1 = LakeLevel / 2;
    }

    for (int t = 0; t < Lim1; t++)
    {
        int  x = RandomRange(0, SimWidth - 21) + 10;
        int y = RandomRange(0, SimHeight - 20) + 10;

        Lim2 = RandomRange(0, 12) + 2;

        for (int z = 0; z < Lim2; z++)
        {
            MapX = x - 6 + RandomRange(0, 12);
            MapY = y - 6 + RandomRange(0, 12);

            if (RandomRange(0, 4))
            {
                SRivPlop();
            }
            else
            {
                BRivPlop();
            }
        }
    }
}


void GetRandStart()
{
    XStart = 40 + RandomRange(0, SimWidth - 80);
    YStart = 33 + RandomRange(0, SimHeight - 67);
    MapX = XStart;
    MapY = YStart;
}


void SmoothWater()
{
    for (int x = 0; x < SimWidth; x++)
    {
        for (int y = 0; y < SimHeight; y++)
        {
            /* If water: */
            if (((Map[x][y] & LOMASK) >= WATER_LOW) && ((Map[x][y] & LOMASK) <= WATER_HIGH))
            {
                if (x > 0)
                {
                    /* If nearest object is not water: */
                    if (((Map[x - 1][y] & LOMASK) < WATER_LOW) || ((Map[x - 1][y] & LOMASK) > WATER_HIGH))
                    {
                        goto edge;
                    }
                }
                if (x < (SimWidth - 1))
                {
                    /* If nearest object is not water: */
                    if (((Map[x + 1][y] & LOMASK) < WATER_LOW) || ((Map[x + 1][y] & LOMASK) > WATER_HIGH))
                    {
                        goto edge;
                    }
                }
                if (y > 0)
                {
                    /* If nearest object is not water: */
                    if (((Map[x][y - 1] & LOMASK) < WATER_LOW) || ((Map[x][y - 1] & LOMASK) > WATER_HIGH))
                    {
                        goto edge;
                    }
                }
                if (y < (SimHeight - 1))
                {
                    /* If nearest object is not water: */
                    if (((Map[x][y + 1] & LOMASK) < WATER_LOW) || ((Map[x][y + 1] & LOMASK) > WATER_HIGH))
                    {
                    edge:
                        Map[x][y] = REDGE; /* set river edge */
                        continue;
                    }
                }
            }
        }
    }

    for (int x = 0; x < SimWidth; x++)
    {
        for (int y = 0; y < SimHeight; y++)
        {
            /* If water which is not a channel: */
            if (((Map[x][y] & LOMASK) != CHANNEL) && ((Map[x][y] & LOMASK) >= WATER_LOW) && ((Map[x][y] & LOMASK) <= WATER_HIGH))
            {
                if (x > 0)
                {
                    /* If nearest object is not water; */
                    if (((Map[x - 1][y] & LOMASK) < WATER_LOW) || ((Map[x - 1][y] & LOMASK) > WATER_HIGH))
                    {
                        continue;
                    }
                }
                if (x < (SimWidth - 1))
                {
                    /* If nearest object is not water: */
                    if (((Map[x + 1][y] & LOMASK) < WATER_LOW) || ((Map[x + 1][y] & LOMASK) > WATER_HIGH))
                    {
                        continue;
                    }
                }
                if (y > 0)
                {
                    /* If nearest object is not water: */
                    if (((Map[x][y - 1] & LOMASK) < WATER_LOW) || ((Map[x][y - 1] & LOMASK) > WATER_HIGH))
                    {
                        continue;
                    }
                }
                if (y < (SimHeight - 1))
                {
                    /* If nearest object is not water: */
                    if (((Map[x][y + 1] & LOMASK) < WATER_LOW) || ((Map[x][y + 1] & LOMASK) > WATER_HIGH))
                    {
                        continue;
                    }
                }
                Map[x][y] = RIVER; /* make it a river */
            }
        }
    }

    for (int x = 0; x < SimWidth; x++)
    {
        for (int y = 0; y < SimHeight; y++)
        {
            /* If woods: */
            if (((Map[x][y] & LOMASK) >= WOODS_LOW) && ((Map[x][y] & LOMASK) <= WOODS_HIGH))
            {
                if (x > 0)
                {
                    /* If nearest object is water: */
                    if ((Map[x - 1][y] == RIVER) || (Map[x - 1][y] == CHANNEL))
                    {
                        Map[x][y] = REDGE; /* make it water's edge */
                        continue;
                    }
                }
                if (x < (SimWidth - 1))
                {
                    /* If nearest object is water: */
                    if ((Map[x + 1][y] == RIVER) || (Map[x + 1][y] == CHANNEL))
                    {
                        Map[x][y] = REDGE; /* make it water's edge */
                        continue;
                    }
                }
                if (y > 0)
                {
                    /* If nearest object is water: */
                    if ((Map[x][y - 1] == RIVER) || (Map[x][y - 1] == CHANNEL))
                    {
                        Map[x][y] = REDGE; /* make it water's edge */
                        continue;
                    }
                }
                if (y < (SimHeight - 1))
                {
                    /* If nearest object is water; */
                    if ((Map[x][y + 1] == RIVER) || (Map[x][y + 1] == CHANNEL))
                    {
                        Map[x][y] = REDGE; /* make it water's edge */
                        continue;
                    }
                }
            }
        }
    }
}


void GenerateMap(int r)
{
    
    if (CreateIsland < 0)
    {
        if (RandomRange(0, 100) < 10) // chance that island is generated
        {
            MakeIsland();
            return;
        }
    }

    
    if (CreateIsland == 1)
    {
        MakeNakedIsland();
    }
    else
    {
        ClearMap();
    }

    GetRandStart();

    if (CurveLevel != 0)
    {
        DoRivers();
    }

    if (LakeLevel != 0)
    {
        MakeLakes();
    }

    SmoothRiver();

    if (TreeLevel != 0)
    {
        DoTrees();
    }
}

#include <iostream>
#include "s_alloc.h"

void GenerateSomeCity(int seed)
{
    ScenarioID = 0;
    CityTime = 0;
    InitSimLoad = 2;
    DoInitialEval = 0;

    InitWillStuff();
    ResetMapState();
    ResetEditorState();
    InvalidateEditors();
    InvalidateMaps();
    UpdateFunds();
    DoSimInit();
    Eval("UIDidGenerateNewCity");
    Kick();

    GenerateMap(seed);
}


void GenerateNewCity()
{
    GenerateSomeCity(RandomRange(0, std::numeric_limits<int>::max()));
}
