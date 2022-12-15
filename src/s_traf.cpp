// This file is part of Micropolis-SDL2PP
// Micropolis-SDL2PP is based on Micropolis
//
// Copyright � 2022 Leeor Dicker
//
// Portions Copyright � 1989-2007 Electronic Arts Inc.
//
// Micropolis-SDL2PP is free software; you can redistribute it and/or modify
// it under the terms of the GNU GPLv3, with additional terms. See the README
// file, included in this distribution, for details.
#include "s_traf.h"

#include "main.h"

#include "Map.h"
#include "Power.h"

#include "s_alloc.h"
#include "s_sim.h"

#include "w_util.h"

#include "Sprite.h"

#include <algorithm>
#include <array>
#include <stack>


namespace
{
    constexpr auto MaxDistance = 30;

    std::stack<Point<int>> CoordinatesStack;

    int Zsource;

    const std::array<Vector<int>, 12> ZonePerimeterOffset =
    { {
        { -1, -2 },
        {  0, -2 },
        {  1, -2 },
        {  2, -1 },
        {  2,  0 },
        {  2,  1 },
        {  1,  2 },
        {  0,  2 },
        { -1,  2 },
        { -2,  1 },
        { -2,  0 },
        { -2, -1 }
    } };

    const std::array<Vector<int>, 4> AdjacentVector
    { {
        {  0, -1 },
        {  1,  0 },
        {  0,  1 },
        { -1,  0 }
    } };


    void pushCoordinates(const Point<int> coordinates)
    {
        CoordinatesStack.push(coordinates);
    }

    void popCoordinates()
    {
        CoordinatesStack.pop();
    }

    void resetCoordinatesStack()
    {
        while (!CoordinatesStack.empty())
        {
            CoordinatesStack.pop();
        }
    }
}


void updateTrafficDensityMap()
{
    while (!CoordinatesStack.empty())
    {
        popCoordinates();
        if (CoordinatesValid(SimulationTarget))
        {
            int tile = maskedTileValue(SimulationTarget.x, SimulationTarget.y);
            if ((tile >= ROADBASE) && (tile < POWERBASE))
            {
                /* check for rail */
                const Point<int> trafficDensityMapCoordinates = SimulationTarget.skewInverseBy({ 2, 2 });
                tile = TrafficDensityMap.value(trafficDensityMapCoordinates);
                tile += 50;

                if ((tile > ResidentialBase) && (RandomRange(0, 5) == 0))
                {
                    tile = ResidentialBase;

                    SimSprite* sprite = getSprite(SimSprite::Type::Helicopter);
                    if (sprite)
                    {
                        sprite->destination = SimulationTarget.skewBy({ 16, 16 });
                    }
                }

                TrafficDensityMap.value(trafficDensityMapCoordinates) = tile;
            }
        }
    }
}


bool RoadTest(const int x)
{
    int tile = x & LOMASK;
    if (tile < ROADBASE)
    {
        return false;
    }
    if (tile > LASTRAIL)
    {
        return false;
    }
    if ((tile >= POWERBASE) && (tile < RAILHPOWERV))
    {
        return false;
    }
    return true;
}


/* look for road on edges of zone   */
bool roadOnZonePerimeter()
{
    for (int i{}; i < ZonePerimeterOffset.size(); ++i)
    {
        const Point<int> coordinates = SimulationTarget + ZonePerimeterOffset[i];
        if (CoordinatesValid(coordinates))
        {
            if (RoadTest(tileValue(coordinates)))
            {
                SimulationTarget = coordinates;
                return true;
            }
        }
    }

    return false;
}


int GetFromMap(int x)
{
    const Point<int> coordinates{ SimulationTarget + AdjacentVector[x] };

    if (!CoordinatesValid(coordinates))
    {
        return 0;
    }

    return maskedTileValue(coordinates);
}


bool TryGo(int distance)
{
    int lastDirection = 5;
    const int startDirection = RandomRange(0, 3);

    for (int direction = startDirection; direction < (startDirection + 4); direction++)
    {
        const int realDirection = direction % 4;

        // skip last direction
        if (realDirection == lastDirection)
        {
            continue;
        }

        if (RoadTest(GetFromMap(realDirection)))
        {
            MoveSimulationTarget(static_cast<SearchDirection>(realDirection));
            lastDirection = (realDirection + 2) % 4;

            // save coordinates every other move
            if (distance % 2)
            {
                pushCoordinates(SimulationTarget);
            }

            return true;
        }
    }

    return false;
}


bool DriveDone()
{
    static int TARGL[3] = { COMBASE, LHTHR, LHTHR };
    static int TARGH[3] = { NUCLEAR, PORT, COMBASE };	/* for destinations */
    //int l, h;

    for (int x = 0; x < 4; x++) /* R>C C>I I>R  */
    {
        int z = GetFromMap(x);
        if ((z >= TARGL[Zsource]) && (z <= TARGH[Zsource]))
        {
            return true;
        }
    }

    return false;
}


bool TryDrive()
{
    for (int distance{}; distance < MaxDistance; ++distance)
    {
        // if it got a road
        if (TryGo(distance))
        {
            if (DriveDone())
            {
                return true;
            }
        }
        else
        {
            // deadend, backup
            if (!CoordinatesStack.empty())
            {
                distance += 3;
            }
            else // give up at start
            {
                return false;
            }
        }
    }

    // MaxDistance reached
    return false;
}


TrafficResult makeTraffic(int Zt)
{
    const auto simLocation = SimulationTarget;

    Zsource = Zt;
    resetCoordinatesStack();

    if (roadOnZonePerimeter()) // look for road on zone perimeter
    {
        if (TryDrive()) // attempt to drive somewhere
        {
            updateTrafficDensityMap(); // if sucessful, inc trafdensity
            SimulationTarget = simLocation;
            return TrafficResult::RouteFound; // traffic passed
        }

        SimulationTarget = simLocation;
        return TrafficResult::RouteNotFound; // traffic failed
    }
    else // no road found
    {
        return TrafficResult::NoTransportNearby;
    }
}
