// This file is part of Micropolis-SDL2PP
// Micropolis-SDL2PP is based on Micropolis
//
// Copyright © 2022 Leeor Dicker
//
// Portions Copyright © 1989-2007 Electronic Arts Inc.
//
// Micropolis-SDL2PP is free software; you can redistribute it and/or modify
// it under the terms of the GNU GPLv3, with additional terms. See the README
// file, included in this distribution, for details.
#include "Map.h"

#include "Budget.h"
#include "Connection.h"
#include "Tool.h"

#include "s_alloc.h"
#include "s_msg.h"
#include "s_sim.h"

#include "w_resrc.h"
#include "w_sound.h"
#include "w_tk.h"
#include "w_update.h"
#include "w_util.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <stdexcept>

int specialBase = CHURCH;

namespace
{
    Tool PendingTool{ Tool::None };

    std::map<Tool, ToolProperties> Tools =
    {
        { Tool::Residential, { 100, 3, 1, false, "Residential" }},
        { Tool::Commercial, { 100, 3, 1, false, "Commercial" }},
        { Tool::Industrial, { 100, 3, 1, false, "Industrial" }},
        { Tool::Fire, { 500, 3, 1, false, "Fire Department" }},
        { Tool::Query, { 0, 1, 0, false, "Query" }},
        { Tool::Police, { 500, 3, 1, false, "Police Department" }},
        { Tool::Wire, { 5, 1, 0, true, "Power Line" }},
        { Tool::Bulldoze, { 1, 1, 0, false, "Bulldoze" }},
        { Tool::Rail, { 20, 1, 0, true, "Rail" }},
        { Tool::Road, { 10, 1, 0, true, "Roads" }},
        { Tool::Stadium, { 5000, 4, 1, false, "Stadium" }},
        { Tool::Park, { 10, 1, 0, false, "Park" }},
        { Tool::Seaport, { 3000, 4, 1, false, "Seaport" }},
        { Tool::Coal, { 3000, 4, 1, false, "Coal Power" }},
        { Tool::Nuclear, { 5000, 4, 1, false, "Nuclear Power" }},
        { Tool::Airport, { 10000, 6, 1, false, "Airport" }},
        { Tool::Network, { 100, 1, 0, false, "Network" }},
        { Tool::None, { 0, 0, 0, false, "No Tool" } }
    };


    std::map<ToolResult, std::string> ToolResultStringTable =
    {
        { ToolResult::CannotBulldoze, "Cannot Bulldoze" },
        { ToolResult::InsufficientFunds, "Insufficient Funds" },
        { ToolResult::InvalidLocation, "Invalid Location" },
        { ToolResult::InvalidOperation, "Invalid Operation" },
        { ToolResult::NetworkVotedNo, "Other players vetoed action" },
        { ToolResult::OutOfBounds, "Out of Bounds" },
        { ToolResult::RequiresBulldozing, "Bulldozing Required" },
        { ToolResult::Success, "Success!" }
    };

    Point<int> ToolStart{};
    Point<int> ToolEnd{};
};


Tool pendingTool()
{
    return PendingTool;
}


void pendingTool(const Tool tool)
{
    PendingTool = tool;
}


const ToolProperties& toolProperties(const Tool tool)
{
    return Tools.at(tool);
}


const ToolProperties& pendingToolProperties()
{
    return Tools.at(PendingTool);
}


void toolStart(const Point<int>& start)
{
    ToolStart = start;
}


const Point<int>& toolStart()
{
    return ToolStart;
}


void toolEnd(const Point<int>& end)
{
    ToolEnd = end;
}


const Point<int>& toolEnd()
{
    return ToolEnd;
}


ToolResult putDownPark(int mapH, int mapV, Budget& budget)
{
    int tile{};

    if (budget.CanAfford(Tools.at(Tool::Park).cost))
    {
        int value = RandomRange(0, 4);

        if (value == 4)
        {
            tile = FOUNTAIN | BURNBIT | BULLBIT | ANIMBIT;
        }
        else
        {
            tile = (value + WOODS2) | BURNBIT | BULLBIT;
        }

        if (Map[mapH][mapV] == 0)
        {
            budget.Spend(Tools.at(Tool::Park).cost);
            UpdateFunds(budget);
            Map[mapH][mapV] = tile;
            return ToolResult::Success;
        }

        return ToolResult::InvalidLocation;
    }

    return ToolResult::InsufficientFunds;
}


// Radar?
ToolResult putDownNetwork(int mapH, int mapV, Budget& budget)
{
    int tile = Map[mapH][mapV] & LOMASK;

    if ((budget.CurrentFunds() > 0) && tally(tile))
    {
        Map[mapH][mapV] = tile = 0;
        budget.Spend(1);
    }

    if (tile != 0)
    {
        return ToolResult::RequiresBulldozing;
    }

    if (budget.CanAfford(Tools.at(Tool::Network).cost))
    {
        Map[mapH][mapV] = TELEBASE | CONDBIT | BURNBIT | BULLBIT | ANIMBIT;
        budget.Spend(Tools.at(Tool::Network).cost);
        return ToolResult::Success;
    }
    else
    {
        return ToolResult::InsufficientFunds;
    }

}


int checkBigZone(int id, int* deltaHPtr, int* deltaVPtr)
{
    switch (id)
    {
    case POWERPLANT: // check coal plant
    case PORT: // check sea port
    case NUCLEAR: // check nuc plant
    case STADIUM: // check stadium
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return (4);

    case POWERPLANT + 1: // check coal plant
    case COALSMOKE3: // check coal plant, smoke
    case COALSMOKE3 + 1: // check coal plant, smoke
    case COALSMOKE3 + 2: // check coal plant, smoke
    case PORT + 1: // check sea port
    case NUCLEAR + 1: // check nuc plant
    case STADIUM + 1: // check stadium
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return (4);

    case POWERPLANT + 4: // check coal plant
    case PORT + 4: // check sea port
    case NUCLEAR + 4: // check nuc plant
    case STADIUM + 4: // check stadium
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return (4);

    case POWERPLANT + 5: // check coal plant
    case PORT + 5: // check sea port
    case NUCLEAR + 5: // check nuc plant
    case STADIUM + 5: // check stadium
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return (4);

        // check airport
        //*** first row ***
    case AIRPORT:
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return (6);

    case AIRPORT + 1:
        *deltaHPtr = -1;
        *deltaVPtr = 0;
        return (6);

    case AIRPORT + 2:
        *deltaHPtr = -2;
        *deltaVPtr = 0;
        return (6);

    case AIRPORT + 3:
        *deltaHPtr = -3;
        *deltaVPtr = 0;
        return (6);

        //*** second row ***
    case AIRPORT + 6:
        *deltaHPtr = 0;
        *deltaVPtr = -1;
        return (6);

    case AIRPORT + 7:
        *deltaHPtr = -1;
        *deltaVPtr = -1;
        return (6);

    case AIRPORT + 8:
        *deltaHPtr = -2;
        *deltaVPtr = -1;
        return (6);

    case AIRPORT + 9:
        *deltaHPtr = -3;
        *deltaVPtr = -1;
        return (6);

        //*** third row ***
    case AIRPORT + 12:
        *deltaHPtr = 0;
        *deltaVPtr = -2;
        return (6);

    case AIRPORT + 13:
        *deltaHPtr = -1;
        *deltaVPtr = -2;
        return (6);

    case AIRPORT + 14:
        *deltaHPtr = -2;
        *deltaVPtr = -2;
        return (6);

    case AIRPORT + 15:
        *deltaHPtr = -3;
        *deltaVPtr = -2;
        return (6);

        //*** fourth row ***
    case AIRPORT + 18:
        *deltaHPtr = 0;
        *deltaVPtr = -3;
        return (6);

    case AIRPORT + 19:
        *deltaHPtr = -1;
        *deltaVPtr = -3;
        return (6);

    case AIRPORT + 20:
        *deltaHPtr = -2;
        *deltaVPtr = -3;
        return (6);

    case AIRPORT + 21:
        *deltaHPtr = -3;
        *deltaVPtr = -3;
        return (6);

    default:
        *deltaHPtr = 0;
        *deltaVPtr = 0;
        return (0);
    }
}


bool tally(int tileValue)
{
    /* can we autobulldoze this tile? */
    return (((tileValue >= FIRSTRIVEDGE) && (tileValue <= LASTRUBBLE)) ||
        ((tileValue >= (POWERBASE + 2)) && (tileValue <= (POWERBASE + 12))) ||
        ((tileValue >= TINYEXP) && (tileValue <= (LASTTINYEXP + 2))));/* ??? */
}


int checkSize(int temp)
{
    /* check for the normal com, resl, ind 3x3 zones & the fireDept & PoliceDept */
    if (((temp >= (ResidentialBase - 1)) && (temp <= (PORTBASE - 1))) ||
        ((temp >= (LASTPOWERPLANT + 1)) && (temp <= (POLICESTATION + 4))))
    {
        return (3);
    }
    else if (((temp >= PORTBASE) && (temp <= LASTPORT)) ||
        ((temp >= COALBASE) && (temp <= LASTPOWERPLANT)) ||
        ((temp >= STADIUMBASE) && (temp <= LASTZONE)))
    {
        return (4);
    }
    return (0);
}


void doConnectTile(const int x, const int y, const int w, const int h, Budget& budget)
{
    if (CoordinatesValid({ x, y }))
    {
        ConnectTile(x, y, Tool::None, budget);
    }
}


void checkBorder(const int mapX, const int mapY, const int count, Budget& budget)
{
    int xPos{}, yPos{};

    xPos = mapX; yPos = mapY - 1;
    for (int cnt = 0; cnt < count; cnt++)
    {
        /*** this will do the upper bordering row ***/
        doConnectTile(xPos, yPos, SimWidth, SimHeight, budget);
        xPos++;
    }

    xPos = mapX - 1; yPos = mapY;
    for (int cnt = 0; cnt < count; cnt++)
    {
        /*** this will do the left bordering row ***/
        doConnectTile(xPos, yPos, SimWidth, SimHeight, budget);
        yPos++;
    }

    xPos = mapX; yPos = mapY + count;
    for (int cnt = 0; cnt < count; cnt++)
    {
        /*** this will do the bottom bordering row ***/
        doConnectTile(xPos, yPos, SimWidth, SimHeight, budget);
        xPos++;
    }

    xPos = mapX + count; yPos = mapY;
    for (int cnt = 0; cnt < count; cnt++)
    {
        /*** this will do the right bordering row ***/
        doConnectTile(xPos, yPos, SimWidth, SimHeight, budget);
        yPos++;
    }
}


ToolResult checkArea(const int mapH, const int mapV, const int base, const int size, const bool animate, const Tool tool, Budget& budget)
{
    if (!pointInRect({ mapH - 1, mapV - 1 }, { 0, 0, SimWidth - size, SimHeight - size }))
    {
        return ToolResult::OutOfBounds;
    }

    bool needsBulldozing{ false };

    int totalCost{ Tools.at(tool).cost };

    int mapY{ mapV - 1 };
    for (int row = 0; row < size; ++row)
    {
        int mapX{ mapH - 1 };

        for (int col = 0; col < size; ++col)
        {
            const unsigned int tileValue{ maskedTileValue(mapX, mapY) };

            if (AutoBulldoze)
            {
                // if autoDoze is enabled, add up the cost of bulldozed tiles
                if (tileValue != 0)
                {
                    if (tally(tileValue))
                    {
                        ++totalCost;
                    }
                    else
                    {
                        needsBulldozing = true;
                    }
                }
            }
            else
            {
                // check and see if the tile is clear or not
                if (tileValue != 0)
                {
                    needsBulldozing = true;
                    break;
                }
            }
            ++mapX;
        }
        ++mapY;
    }

    if (needsBulldozing)
    {
        return ToolResult::RequiresBulldozing;
    }

    if (!budget.CanAfford(totalCost))
    {
        return ToolResult::InsufficientFunds;
    }

    budget.Spend(totalCost);
    UpdateFunds(budget);

    int tileBase = base;
    mapY = mapV - 1;
    for (int row = 0; row < size; ++row)
    {
        int mapX = mapH - 1;

        for (int col = 0; col < size; ++col)
        {
            if (col == 1 && row == 1)
            {
                Map[mapX][mapY] = tileBase + BNCNBIT + ZONEBIT;
            }
            // special case to get nuclear plant animation working
            else if (animate && col == 1 && row == 2)
            {
                Map[mapX][mapY] = tileBase + BNCNBIT + ANIMBIT;
            }
            else
            {
                Map[mapX][mapY] = tileBase + BNCNBIT;
            }
            ++mapX;
            ++tileBase;
        }
        ++mapY;
    }

    checkBorder(mapH - 1, mapV - 1, size, budget);
    return ToolResult::Success;
}


/* QUERY */
/* search table for zone status string match */
static int idArray[28] =
{
    DIRT,
    RIVER,
    TREEBASE,
    RUBBLE,
    FLOOD,
    RADTILE,
    FIRE,
    ROADBASE,
    POWERBASE,
    RAILBASE,
    ResidentialBase,
    COMBASE,
    INDBASE,
    PORTBASE,
    AIRPORTBASE,
    COALBASE,
    FIRESTBASE,
    POLICESTBASE,
    STADIUMBASE,
    NUCLEARBASE,
    827, // crossed out lightning bolt
    832, // radar dish first frame
    FOUNTAIN,
    INDBASE2,
    FOOTBALLGAME1,
    VBRDG0,
    952, // radiation icon first frame
    956 // white tile?
};

/*
  0, 2, 21, 44, 
  48, 52, 53, 64,
  208, 224, 240, 423, 
  612, 693, 709, 745,
  761, 770, 779, 811, 
  827, 832, 840, 844,
  932, 948, 952, 956

  Clear, Water, Trees, Rubble, 
  Flood, Radioactive Waste, Fire, Road,
  Power, Rail, Residential, Commercial,
  Industrial, Port, AirPort, Coal Power,
  Fire Department, Police Department, Stadium, Nuclear Power, 
  Draw Bridge, Radar Dish, Fountain, Industrial,
  49er's 38  Bears 3, Draw Bridge, Ur 238
*/


int getDensityStr(int catNo, int mapH, int mapV)
{
    int z;

    switch (catNo)
    {
    case 0:
        z = PopulationDensityMap.value({ mapH >> 1, mapV >> 1 });
        z = z >> 6;
        z = z & 3;
        return (z);

    case 1:
        z = LandValueMap.value({ mapH >> 1, mapV >> 1 });
        if (z < 30) return (4);
        if (z < 80) return (5);
        if (z < 150) return (6);
        return (7);

    case 2:
        z = CrimeMap.value({ mapH >> 1, mapV >> 1 });
        z = z >> 6;
        z = z & 3;
        return (z + 8);

    case 3:
        z = PollutionMap.value({ mapH >> 1, mapV >> 1 });
        if ((z < 64) && (z > 0)) return (13);
        z = z >> 6;
        z = z & 3;
        return (z + 12);

    case 4:
        z = RateOfGrowthMap.value({ mapH >> 3, mapV >> 3 });
        if (z < 0) return (16);
        if (z == 0) return (17);
        if (z > 100) return (19);
        return (18);

    default:
        throw std::runtime_error("");
    }
}


struct ZoneStatsStrings
{
    const std::string str;
    const std::string s0;
    const std::string s1;
    const std::string s2;
    const std::string s3;
    const std::string s4;
};


void DoShowZoneStatus(const ZoneStatsStrings zoneStats, int x, int y)
{
    const std::string msg{
        "UIShowZoneStatus {" +
        zoneStats.str + "} {" +
        zoneStats.s0 + "} {" +
        zoneStats.s1 + "} {" +
        zoneStats.s2 + "} {" +
        zoneStats.s3 + "} {" +
        zoneStats.s4 + "} {" +
        std::to_string(x) + "} {" +
        std::to_string(y) + "}"
    };

    Eval(msg);
}


const std::string& queryString(int tileValue)
{
    for (int i = 1; i < 29; ++i)
    {
        if (tileValue < idArray[i])
        {
            /*
            int queryId = i - 1;
            if (queryId < 1 || queryId > 28)
            {
                queryId = 28;
            }
            */

            int queryId = std::clamp(i - 1, 0, 28);

            // \fixme ugly cast
            return QueryStatsString(static_cast<QueryStatsId>(queryId));

            break;
        }
    }

    return QueryStatsString(QueryStatsId::Padding2);
}


void doZoneStatus(int x, int y)
{
    int tileNum = maskedTileValue(x, y);
    if (tileNum >= COALSMOKE1 && tileNum < FOOTBALLGAME1)
    {
        tileNum = COALBASE;
    }

    std::string localStr = queryString(tileNum);
    std::array<std::string, 5> statusStr;

    for (int _x = 0; _x < 5; ++_x)
    {
        int id = getDensityStr(_x, x, y);
        id++;
        
        // \fixme ugly cast
        statusStr[_x] = ZoneStatsString(static_cast<ZoneStatsId>(std::clamp(id, 1, 19)));
    }

    DoShowZoneStatus({ localStr, statusStr[0], statusStr[1], statusStr[2], statusStr[3], statusStr[4] }, x, y);
}


void putRubble(const int mapX, const int mapY, const int size)
{
    for (int x = mapX - 1; x < mapX + size - 1; x++)
    {
        for (int y = mapY - 1; y < mapY + size - 1; y++)
        {
            if (CoordinatesValid({ x, y }))
            {
                int cellValue = maskedTileValue(x, y);
                if ((cellValue != RADTILE) && (cellValue != 0))
                {
                    Map[x][y] = (animationEnabled() ? (TINYEXP + RandomRange(0, 2)) : SOMETINYEXP) | ANIMBIT | BULLBIT;
                }
            }
        }
    }
}


/************************************************************************/
/* TOOLS */


ToolResult query_tool(int x, int y, Budget&)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    doZoneStatus(x, y);
    return ToolResult::Success;
}


ToolResult bulldozer_tool(int x, int y, Budget& budget)
{
    unsigned int currTile, temp;
    int zoneSize, deltaH, deltaV;

    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    currTile = Map[x][y];
    temp = currTile & LOMASK;

    ToolResult result = ToolResult::Success;

    if (currTile & ZONEBIT)
    { /* zone center bit is set */
        if (!budget.Broke())
        {
            budget.Spend(1);
            switch (checkSize(temp))
            {
            case 3:
                MakeSound("city", "Explosion-High");
                putRubble(x, y, 3);
                break;

            case 4:
                putRubble(x, y, 4);
                MakeSound("city", "Explosion-Low");
                break;

            case 6:
                MakeSound("city", "Explosion-High");
                MakeSound("city", "Explosion-Low");
                putRubble(x, y, 6);
                break;

            default:
                break;
            }
        }
    }
    else if ((zoneSize = checkBigZone(temp, &deltaH, &deltaV)))
    {
        if (!budget.Broke())
        {
            budget.Spend(1);
            switch (zoneSize)
            {
            case 3:
                MakeSound("city", "Explosion-High");
                break;

            case 4:
                MakeSound("city", "Explosion-Low");
                putRubble(x + deltaH, y + deltaV, 4);
                break;

            case 6:
                MakeSound("city", "Explosion-High");
                MakeSound("city", "Explosion-Low");
                putRubble(x + deltaH, y + deltaV, 6);
                break;
            }
        }
    }
    else
    {
        if (temp == RIVER || temp == REDGE || temp == CHANNEL)
        {
            if (budget.CanAfford(5)) /// \fixme Magic Number
            {
                result = ConnectTile(x, y, Tool::Bulldoze, budget);
                if (temp != (Map[x][y] & LOMASK))
                {
                    budget.Spend(5);
                }
            }
            else
            {
                result = ToolResult::InsufficientFunds;
            }
        }
        else
        {
            result = ConnectTile(x, y, Tool::Bulldoze, budget);
        }
    }
    UpdateFunds(budget);
    return result;
}


ToolResult road_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    ToolResult result = ConnectTile(x, y, Tool::Road, budget);
    UpdateFunds(budget);
    return result;
}


ToolResult rail_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    ToolResult result = ConnectTile(x, y, Tool::Rail, budget);
    UpdateFunds(budget);
    return result;
}


ToolResult wire_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    ToolResult result = ConnectTile(x, y, Tool::Wire, budget);
    UpdateFunds(budget);
    return result;
}


ToolResult park_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return putDownPark(x, y, budget);
}


ToolResult residential_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, ResidentialBase, 3, false, Tool::Residential, budget);
}


ToolResult commercial_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, COMBASE, 3, false, Tool::Commercial, budget);
}


ToolResult industrial_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, INDBASE, 3, false, Tool::Industrial, budget);
}


ToolResult police_dept_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, POLICESTBASE, 3, false, Tool::Police, budget);
}


ToolResult fire_dept_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, FIRESTBASE, 3, false , Tool::Fire, budget);
}


ToolResult stadium_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, STADIUMBASE, 4, false, Tool::Stadium, budget);
}


ToolResult coal_power_plant_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, COALBASE, 4, false, Tool::Coal, budget);
}


ToolResult nuclear_power_plant_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, NUCLEARBASE, 4, true, Tool::Nuclear, budget);
}


ToolResult seaport_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, PORTBASE, 4, false, Tool::Seaport, budget);
}


ToolResult airport_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return checkArea(x, y, AIRPORTBASE, 6, false, Tool::Airport, budget);
}


ToolResult network_tool(int x, int y, Budget& budget)
{
    if (!CoordinatesValid({ x, y }))
    {
        return ToolResult::OutOfBounds;
    }

    return putDownNetwork(x, y, budget);
}


std::map<Tool, ToolResult(*)(int, int, Budget&)> ToolFunctionTable =
{
    { Tool::Residential, &residential_tool },
    { Tool::Commercial, &commercial_tool },
    { Tool::Industrial, &industrial_tool },
    { Tool::Fire, &fire_dept_tool },
    { Tool::Query, &query_tool },
    { Tool::Police, &police_dept_tool },
    { Tool::Wire, &wire_tool },
    { Tool::Bulldoze, &bulldozer_tool },
    { Tool::Rail, &rail_tool },
    { Tool::Road, &road_tool },
    { Tool::Stadium, &stadium_tool },
    { Tool::Park, &park_tool },
    { Tool::Seaport, &seaport_tool },
    { Tool::Coal, &coal_power_plant_tool },
    { Tool::Nuclear, &nuclear_power_plant_tool },
    { Tool::Airport, &airport_tool },
    { Tool::Network, &network_tool },
    { Tool::None, nullptr }
};


/**
 * Performs tool action
 * 
 * Coordinates expected to be in tile coords,
 * not screen coords.
 */
void ToolDown(const Point<int> location, Budget& budget)
{
    if (PendingTool == Tool::None)
    {
        return;
    }

    ToolResult result = ToolFunctionTable.at(PendingTool)(location.x, location.y, budget);

    if (result == ToolResult::RequiresBulldozing)
    {
        ClearMes();
        SendMes(NotificationId::MustBulldoze);
        MakeSoundOn(nullptr, "edit", "UhUh");
    }
    else if (result == ToolResult::InsufficientFunds)
    {
        ClearMes();
        SendMes(NotificationId::InsufficientFunds);
        MakeSoundOn(nullptr, "edit", "Sorry");
    }
}


int longestAxis(const Vector<int>& vec)
{
    return abs(vec.x) >= abs(vec.y) ? vec.x : vec.y;
}


/**
 * Modifies \c toolVector
 */
void validateDraggableToolVector(Vector<int>& toolVector, Budget& budget)
{
    const int axisLarge = longestAxis(toolVector);
    if (axisLarge == 0) { return; }

    const int step = axisLarge < 0 ? -1 : 1;
    const bool xAxisLarger = std::abs(toolVector.x) > std::abs(toolVector.y);

    const Point<int>& origin = toolStart();
    
    if (CanConnectTile(origin.x, origin.y, pendingTool(), budget) != ToolResult::Success)
    {
        toolVector = {};
        return;
    }

    if (xAxisLarger)
    {
        for (int i = 0; std::abs(i) <= std::abs(toolVector.x); i += step)
        {
            const auto result = CanConnectTile(origin.x + i, origin.y, pendingTool(), budget);
            if (result != ToolResult::Success)
            {
                toolVector = { i - step, 0 };
                return;
            }
        }
    }
    else // ew, find a better way to do this
    {
        for (int i = 0; std::abs(i) <= std::abs(toolVector.y); i += step)
        {
            if (CanConnectTile(origin.x, origin.y + i, pendingTool(), budget) != ToolResult::Success)
            {
                toolVector = { 0, i - step };
                return;
            }
        }
    }
}


void executeDraggableTool(const Vector<int>& toolVector, const Point<int>& tilePointedAt, Budget& budget)
{
    if (toolVector == Vector<int>{ 0, 0 })
    {
        ToolDown(tilePointedAt, budget);
        return;
    }

    const bool xAxisLarger = std::abs(toolVector.x) > std::abs(toolVector.y);
    const int axis = longestAxis(toolVector);
    const int step = axis < 0 ? -1 : 1;

    if (xAxisLarger)
    {
        for (int i = 0; std::abs(i) <= std::abs(toolVector.x); i += step)
        {
            ConnectTile(toolStart().x + i, toolStart().y, pendingTool(), budget);
        }
    }
    else
    {
        for (int i = 0; std::abs(i) <= std::abs(toolVector.y); i += step)
        {
            ConnectTile(toolStart().x, toolStart().y + i, pendingTool(), budget);
        }
    }
}
