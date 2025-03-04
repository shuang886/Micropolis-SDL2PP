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
#include "Sprite.h"

#include "Map.h"
#include "Tool.h"

#include "s_alloc.h"
#include "s_msg.h"
#include "s_sim.h"

#include "Scan.h"

#include "w_sound.h"
#include "w_util.h"

#include <algorithm>
#include <map>
#include <string>

#include <SDL2/SDL.h>


int absDist;
int Cycle;


std::vector<SimSprite> Sprites;


namespace
{
    const std::map<SimSprite::Type, std::string> SpriteTypeToId
    {
        { SimSprite::Type::Train, "1" },
        { SimSprite::Type::Helicopter, "2" },
        { SimSprite::Type::Airplane, "3" },
        { SimSprite::Type::Ship, "4" },
        { SimSprite::Type::Monster, "5" },
        { SimSprite::Type::Tornado, "6" },
        { SimSprite::Type::Explosion, "7" }
    };

    Point<int> CrashPosition{};


    void loadSpriteImages(SimSprite::Type type, int frames, std::vector<Texture>& frameList)
    {
        if(!frameList.empty())
        {
            return;
        }

        for (int i = 0; i < frames; i++)
        {
            std::string name = std::string("images/obj") + SpriteTypeToId.at(type) + "-" + std::to_string(i) + ".xpm";
            frameList.push_back(loadTexture(MainWindowRenderer, name));
        }
    }


    void initSprite(SimSprite& sprite, const Point<int>& position)
    {
        sprite.position = position;
        sprite.origin = {};
        sprite.destination = {};
        sprite.size = {};
        sprite.frame = 0;
        sprite.count = 0;
        sprite.sound_count = 0;
        sprite.dir = 0;
        sprite.new_dir = 0;
        sprite.step = 0;
        sprite.flag = 0;
        sprite.turn = 0;
        sprite.accel = 0;
        sprite.speed = 100;
        sprite.active = true;

        switch (sprite.type)
        {

        case SimSprite::Type::Train:
            sprite.size = { 32, 32 };
            sprite.offset = { 32, -16 };
            sprite.hot = { 40, -8 };
            sprite.frame = 1;
            sprite.dir = 4;
            loadSpriteImages(SimSprite::Type::Train, 5, sprite.frames);
            break;

        case SimSprite::Type::Ship:
            sprite.size = { 48, 48 };
            sprite.offset = { 32, -16 };
            sprite.hot = { 48, 0 };

            if (position.x < 64)
            {
                sprite.frame = 2;
            }
            else if (position.x >= ((SimWidth - 4) * 16))
            {
                sprite.frame = 6;
            }
            else if (position.y < 64)
            {
                sprite.frame = 4;
            }
            else if (position.y >= ((SimHeight - 4) * 16))
            {
                sprite.frame = 0;
            }
            else
            {
                sprite.frame = 2;
            }

            sprite.new_dir = sprite.frame;
            sprite.dir = 0;
            sprite.count = 1;
            loadSpriteImages(SimSprite::Type::Ship, 9, sprite.frames);
            break;

        case SimSprite::Type::Monster:
            sprite.size = { 48, 48 };
            sprite.offset = { 24, 0 };
            sprite.hot = { 40, 16 };
            sprite.destination = { pollutionMax().x * 16, pollutionMax().y * 16 };
            sprite.origin = position;

            if (position.x > ((SimWidth << 4) / 2))
            {
                if (position.y > ((SimHeight << 4) / 2)) sprite.frame = 10;
                else sprite.frame = 7;
            }
            else if (position.y > ((SimHeight << 4) / 2))
            {
                sprite.frame = 1;
            }
            else
            {
                sprite.frame = 4;
            }
            sprite.count = 1000;
            loadSpriteImages(SimSprite::Type::Monster, 17, sprite.frames);
            break;

        case SimSprite::Type::Helicopter:
            sprite.size = { 32, 32 };
            sprite.offset = { 32, -16 };
            sprite.hot = { 40, -8 };
            sprite.destination = { RandomRange(0, SimWidth - 1), RandomRange(0, SimHeight - 1) };
            sprite.origin = position + Vector<int>{ -30, 0 };
            sprite.frame = 5;
            sprite.count = 1500;
            loadSpriteImages(SimSprite::Type::Helicopter, 9, sprite.frames);
            break;

        case SimSprite::Type::Airplane:
            sprite.size = { 48, 48 };
            sprite.offset = { 24, 0 };
            sprite.hot = { 48, 16 };

            sprite.destination =
            {
                RandomRange(0, (SimWidth * 16) + 100) - 50,
                RandomRange(0, (SimHeight * 16) + 100) - 50
            };

            loadSpriteImages(SimSprite::Type::Airplane, 12, sprite.frames);
            break;

        case SimSprite::Type::Tornado:
            sprite.size = { 48, 48 };
            sprite.offset = { 24, 0 };
            sprite.hot = { 40, 36 };
            sprite.frame = 0;
            sprite.count = 200;
            loadSpriteImages(SimSprite::Type::Tornado, 3, sprite.frames);
            break;

        case SimSprite::Type::Explosion:
            sprite.size = { 48, 48 };
            sprite.offset = { 24, 0 };
            sprite.hot = { 40, 16 };
            sprite.frame = 0;
            loadSpriteImages(SimSprite::Type::Explosion, 6, sprite.frames);
            break;

        default:
            throw std::runtime_error("Undefined sprite type");
            break;
        }
    }


    void makeSprite(SimSprite::Type type, const Point<int>& position)
    {
        for (auto& sprite : Sprites)
        {
            if (sprite.type == type)
            {
                sprite.active = true;
                sprite.position = position;
                initSprite(sprite, position);
                return;
            }
        }

        Sprites.push_back(SimSprite());
        Sprites.back().type = type;
        initSprite(Sprites.back(), position);
    }


    void drawSprite(SimSprite& sprite)
    {
        const auto& spriteFrame = sprite.frames[sprite.frame];

        const SDL_Rect dstRect
        {
            sprite.position.x - viewOffset().x + sprite.offset.x,
            sprite.position.y - viewOffset().y + sprite.offset.y,
            spriteFrame.dimensions.x,
            spriteFrame.dimensions.y
        };

        SDL_RenderCopy(MainWindowRenderer, spriteFrame.texture, &spriteFrame.area, &dstRect);
    }

};


Point<int>& crashPosition()
{
    return CrashPosition;
}


void crashPosition(const Point<int>& position)
{
    CrashPosition = position;
}


SimSprite* getSprite(SimSprite::Type type)
{
    for (auto& sprite : Sprites)
    {
        if(sprite.type == type)
        {
            return &sprite;
        }
    }

    return nullptr;
}


void drawSprites()
{
    for (auto& sprite : Sprites)
    {
        if (!sprite.active)
        {
            continue;
        }

        drawSprite(sprite);
    }
}


void destroyAllSprites()
{
    Sprites.clear();
}


int getTile(const Point<int>& location)
{
    const Point<int> coordinate{ location.skewInverseBy({ 16, 16 }) };
 
    if (!CoordinatesValid(coordinate))
    {
        return(-1);
    }
    else
    {
        return maskedTileValue(coordinate.x, coordinate.y);
    }
}


int turnTo(int p, int d)
{
    if (p == d)
    {
        return p;
    }

    if (p < d)
    {
        if ((d - p) < 4)
        {
            p++;
        }
        else
        {
            p--;
        }
    }
    else
    {
        if ((p - d) < 4)
        {
            p--;
        }
        else
        {
            p++;
        }
    }

    if (p > 8)
    {
        p = 1;
    }
    if (p < 1)
    {
        p = 8;
    }

    return p;
}


bool tryOther(int Tpoo, int Told, int Tnew)
{
    int z;

    z = Told + 4;
    if (z > 8)
    {
        z -= 8;
    }

    if (Tnew != z)
    {
        return false;
    }

    if ((Tpoo == POWERBASE) || (Tpoo == POWERBASE + 1) || (Tpoo == RAILBASE) || (Tpoo == RAILBASE + 1))
    {
        return true;
    }

    return false;
}


int spritePositionValid(SimSprite& sprite)
{
    const Point<int> adjustedPoint{ sprite.position + Vector<int>{sprite.hot.x, sprite.hot.y} };
    constexpr SDL_Rect worldArea{ 0, 0, ValidMapCoordinates.w * 16, ValidMapCoordinates.h * 16 };

    return pointInRect(adjustedPoint, worldArea);
}


int getDirection(int orgX, int orgY, int desX, int desY)
{
    static int Gdtab[13] = { 0, 3, 2, 1, 3, 4, 5, 7, 6, 5, 7, 8, 1 };
    int dispX, dispY, z;

    dispX = desX - orgX;
    dispY = desY - orgY;
    if (dispX < 0)
    {
        if (dispY < 0)
        {
            z = 11;
        }
        else
        {
            z = 8;
        }
    }
    else
    {
        if (dispY < 0)
        {
            z = 2;
        }
        else
        {
            z = 5;
        }
    }

    if (dispX < 0)
    {
        dispX = -dispX;
    }

    if (dispY < 0)
    {
        dispY = -dispY;
    }

    absDist = dispX + dispY;

    if ((dispX << 1) < dispY)
    {
        z++;
    }
    else if ((dispY << 1) < dispY)
    {
        z--;
    }

    if ((z < 0) || (z > 12))
    {
        z = 0;
    }

    return Gdtab[z];
}


bool pointInRange(const Point<int>& point1, const Point<int>& point2, const int distance)
{
    return (point2 - point1).lengthSquared() <= distance * distance;
}


bool spritesCollided(SimSprite& s1, SimSprite& s2)
{
    return ((s1.active) && (s2.active) && pointInRange(s1.position, s2.position, 30));
}


bool tileIsWet(int x)
{
    return ((x == POWERBASE) ||
        (x == POWERBASE + 1) ||
        (x == RAILBASE) ||
        (x == RAILBASE + 1) ||
        (x == BRWH) ||
        (x == BRWV));
}


void oFireZone(int Xloc, int Yloc, int ch)
{
    int XYmax;

    const auto rogVal = RateOfGrowthMap.value({ Xloc >> 3, Yloc >> 3 });
    RateOfGrowthMap.value({ Xloc >> 3, Yloc >> 3 }) = rogVal - 20;

    ch &= LOMASK;
    if (ch < PORTBASE)
    {
        XYmax = 2;
    }
    else
    {
        if (ch == AIRPORT)
        {
            XYmax = 5;
        }
        else
        {
            XYmax = 4;
        }
    }

    for (int x = -1; x < XYmax; x++)
    {
        for (int y = -1; y < XYmax; y++)
        {
            const int Xtem = Xloc + x;
            const int Ytem = Yloc + y;
            if ((Map[Xtem][Ytem] & LOMASK) >= ROADBASE)
            {
                Map[Xtem][Ytem] |= BULLBIT;
            }
        }
    }
}


void startFire(const Point<int>& location)
{
    const Point<int> mapCoords = { location.skewInverseBy({16, 16}) };

    if (!CoordinatesValid(mapCoords))
    {
        return;
    }

    const int unmaskedTile = tileValue(mapCoords.x, mapCoords.y);
    const int tile = maskedTileValue(mapCoords.x, mapCoords.y);

    if ((!(unmaskedTile & BURNBIT)) && (tile != 0))
    {
        return;
    }

    if (unmaskedTile & ZONEBIT)
    {
        return;
    }

    Map[mapCoords.x][mapCoords.y] = FIRE + RandomRange(0, 3) + ANIMBIT;
}


void destroyTile(const Point<int>& location)
{
    const Point<int> mapCoords = { location.skewInverseBy({16, 16}) };

    if (!CoordinatesValid(mapCoords))
    {
        return;
    }

    const int unmaskedTile = tileValue(mapCoords.x, mapCoords.y);
    const int tile = maskedTileValue(mapCoords.x, mapCoords.y);

    if (tile >= TREEBASE)
    {
        /* TILE_IS_BRIDGE(t) */
        if (!(unmaskedTile & BURNBIT))
        {
            if ((tile >= ROADBASE) && (tile <= LASTROAD))
            {
                Map[mapCoords.x][mapCoords.y] = RIVER;
                return;
            }
        }
        if (unmaskedTile & ZONEBIT)
        {
            oFireZone(mapCoords.x, mapCoords.y, unmaskedTile);
            if (tile > RZB)
            {
                makeExplosionAt(location);
            }
        }
        if (tileIsWet(tile))
        {
            Map[mapCoords.x][mapCoords.y] = RIVER;
        }
        else
        {
            Map[mapCoords.x][mapCoords.y] = (animationEnabled() ? TINYEXP : (LASTTINYEXP - 3)) | BULLBIT | ANIMBIT;
        }
    }
}


void explodeSprite(SimSprite& sprite)
{
    sprite.active = false;
    Point<int> location{ sprite.position.x + sprite.hot.x, sprite.position.y + sprite.hot.y };
    
    makeExplosionAt(location);

    location = location.skewInverseBy({ 16, 16 });

    switch (sprite.type)
    {
    case SimSprite::Type::Airplane:
        CrashPosition = location;
        SendMesAt(NotificationId::PlaneCrashed, location.x, location.y);
        break;

    case SimSprite::Type::Ship:
        CrashPosition = location;
        SendMesAt(NotificationId::ShipWrecked, location.x, location.y);
        break;

    case SimSprite::Type::Train:
        CrashPosition = location;
        SendMesAt(NotificationId::TrainCrashed, location.x, location.y);
        break;

    case SimSprite::Type::Helicopter:
        CrashPosition = location;
        SendMesAt(NotificationId::HelicopterCrashed, location.x, location.y);
        break;
            
    default:
        break;
    }

    MakeSound("city", "Explosion-High"); /* explosion */
    return;
}


void updateTrain(SimSprite& sprite)
{
    static const std::array<Vector<int>, 4> CheckVector{{ {0, -16}, {16, 0}, {0, 16}, {-16, 0} }};
    static const std::array<Vector<int>, 5> MovementVector{{ {0, -4}, {4, 0}, {0, 4}, {-4, 0}, {0, 0} }};


    static int TrainPic2[5] = { 0, 1, 0, 1, 4 };

    if ((sprite.frame == 2) || (sprite.frame == 3))
    {
        sprite.frame = TrainPic2[sprite.dir];
    }

    sprite.position += MovementVector[sprite.dir];

    int dir = RandomRange(0, 4);
    for (int z = dir; z < (dir + 4); z++)
    {
        int checkDirection = z % 4;

        if (sprite.dir != 4)
        {
            if (checkDirection == ((sprite.dir + 2) % 4))
            {
                continue;
            }
        }

        int c = getTile(sprite.position + CheckVector[checkDirection] + Vector<int>{ 48, 0});

        if (((c >= RAILBASE) && (c <= LASTRAIL)) || /* track? */
            (c == RAILVPOWERH) ||
            (c == RAILHPOWERV))
        {
            if ((sprite.dir != checkDirection) && (sprite.dir != 4))
            {
                if ((sprite.dir + checkDirection) == 3)
                {
                    sprite.frame = 2;
                }
                else
                {
                    sprite.frame = 3;
                }
            }
            else
            {
                sprite.frame = TrainPic2[checkDirection];
            }

            if ((c == RAILBASE) || (c == (RAILBASE + 1)))
            {
                sprite.frame = 4;
            }

            sprite.dir = checkDirection;
            return;
        }
    }

    if (sprite.dir == 4)
    {
        sprite.frame = 0;
        return;
    }

    sprite.dir = 4;
}


void updateHelicopter(SimSprite& sprite)
{
    static const std::array<Vector<int>, 9> CD =
    { {
        { 0, 0 }, { 0, -5 }, { 3, -3 }, { 5, 0 }, { 3, 3 }, { 0, 5 }, { -3, 3 }, { -5, 0 }, { -3, -3 }
    } };

    if (sprite.sound_count > 0)
    {
        sprite.sound_count--;
    }

    if (sprite.count > 0)
    {
        sprite.count--;
    }

    if (!sprite.count)
    {
        // Attract copter to monster and tornado so it blows up more often
        const auto monster = getSprite(SimSprite::Type::Monster);
        if (monster != nullptr)
        {
            sprite.destination = monster->position;
        }
        else
        {
            const auto tornado = getSprite(SimSprite::Type::Tornado);
            if (tornado != nullptr)
            {
                sprite.destination = tornado->destination;
            }
            else
            {
                sprite.destination = sprite.origin;
            }
        }
    }

    if (sprite.count == 0) // land
    {
        getDirection(sprite.position.x, sprite.position.y, sprite.origin.x, sprite.origin.y);

        if (absDist < 30)
        {
            sprite.active = false;
            return;
        }
    }


    if (!sprite.sound_count) // send report
    {
        const Point<int> location{ (sprite.position.x + 48) >> 5, sprite.position.y >> 5 };
        if ((location.x >= 0) && (location.x < (SimWidth >> 1)) && (location.y >= 0) && (location.y < (SimHeight >> 1)))
        {
            // Don changed from 160 to 170 to shut the #$%#$% thing up!
            if ((TrafficDensityMap.value(location) > 170) && (RandomRange(0, 7) == 0))
            {
                SendMesAt(NotificationId::HeavyTrafficReported, (location.x << 1) + 1, (location.y << 1) + 1);
                MakeSound("city", "HeavyTraffic"); // chopper
                sprite.sound_count = 200;
            }
        }
    }

    if (!(Cycle & 3))
    {
        int direction{ getDirection(sprite.position.x, sprite.position.y, sprite.destination.x, sprite.destination.y) };
        sprite.frame = turnTo(sprite.frame, direction);
    }

    sprite.position += CD[sprite.frame];
}


void updateAirplane(SimSprite& sprite)
{
    static const std::array<Vector<int>, 12> CD =
    {{
        {0, 0}, {0, -8}, {6, -6}, {8, 0}, {6, 6}, {0, 8},
        {-6, 6}, {-8, 0}, {-6, -6}, {8, 0}, {8, 0}, {8, 0}
    }};

    int z, d;

    z = sprite.frame;

    if ((Cycle % 5) == 0)
    {
        if (z > 8)  /* TakeOff  */
        {
            z--;
            if (z < 9) z = 3;
            sprite.frame = z;
        }
        else /* goto destination */
        {
            d = getDirection(sprite.position.x, sprite.position.y, sprite.destination.x, sprite.destination.y);
            z = turnTo(z, d);
            sprite.frame = z;
        }
    }

    if (absDist < 50) /* at destination  */
    {
        sprite.destination =
        {
            RandomRange(0, (SimWidth * 16) + 100) - 50,
            RandomRange(0, (SimHeight * 16) + 100) - 50
        };
    }

    /* deh added test for !Disasters */
    if (!NoDisasters)
    {
        for (auto& other : Sprites)
        {
            if (&sprite == &other || !other.active)
            {
                continue;
            }

            if (other.type == SimSprite::Type::Airplane || other.type == SimSprite::Type::Helicopter)
            {
                if(spritesCollided(sprite, other))
                {
                    explodeSprite(sprite);
                    explodeSprite(other);
                }
            }
        }
    }

    sprite.position += CD[z];

    if (!spritePositionValid(sprite))
    {
        sprite.active = false;
    }
}


void updateShip(SimSprite& sprite)
{
    static const std::array<Vector<int>, 9> CheckDirection{ {{0,0}, {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}} };
    static const std::array<Vector<int>, 9> MoveVector{ {{0,0}, {0,-2}, {2,-2}, {2,0}, {2,2}, {0,2}, {-2,2}, {-2,0}, {-2,-2}} };
    static const std::array<int, 8> BtClrTab{ RIVER, CHANNEL, POWERBASE, POWERBASE + 1, RAILBASE, RAILBASE + 1, BRWH, BRWV };

    int t = RIVER;
    int tem, pem;

    if (sprite.sound_count > 0)
    {
        sprite.sound_count--;
    }

    if (!sprite.sound_count)
    {
        if (RandomRange(0, 3) == 1)
        {
            if ((ScenarioID == 2) && /* San Francisco */
                (RandomRange(0, 10) < 5))
            {
                MakeSound("city", "HonkHonk-Low -speed 80");
            }
            else
            {
                MakeSound("city", "HonkHonk-Low");
            }
        }
        sprite.sound_count = 200;
    }

    if (sprite.count > 0)
    {
        sprite.count--;
    }

    if (!sprite.count)
    {
        sprite.count = 9;
        if (sprite.frame != sprite.new_dir)
        {
            sprite.frame = turnTo(sprite.frame, sprite.new_dir);
            return;
        }

        tem = RandomRange(0, 7);
        for (pem = tem; pem < (tem + 8); pem++)
        {
            const int z = (pem & 7) + 1;

            if (z == sprite.dir)
            {
                continue;
            }

            const Point<int> position
            {
                ((sprite.position.x + (sprite.hot.x - 1)) / 16) + CheckDirection[z].x, 
                ((sprite.position.y + sprite.hot.y) / 16) + CheckDirection[z].y
            };
            
            if (CoordinatesValid(position))
            {
                t = maskedTileValue(position.x, position.y);
                if ((t == CHANNEL) || (t == BRWH) || (t == BRWV) || tryOther(t, sprite.dir, z))
                {
                    sprite.new_dir = z;
                    sprite.frame = turnTo(sprite.frame, sprite.new_dir);
                    sprite.dir = z + 4;

                    if (sprite.dir > 8)
                    {
                        sprite.dir -= 8;
                    }

                    break;
                }
            }
        }

        if (pem == (tem + 8))
        {
            sprite.dir = 10;
            sprite.new_dir = RandomRange(0, 7) + 1;
        }
    }
    else
    {
        if (sprite.frame == sprite.new_dir)
        {
            sprite.position += MoveVector[sprite.frame];
        }
    }

    if (!spritePositionValid(sprite))
    {
        sprite.active = false;
        return;
    }

    for (const auto tileValue : BtClrTab)
    {
        if (t == tileValue)
        {
            return;
        }
    }

    explodeSprite(sprite);
    destroyTile(sprite.position + Vector<int>{48, 0});
}


void updateMonster(SimSprite& sprite)
{
    static const std::array<Vector<int>, 5> G =
    {{
        { 2, -2 }, { 2, 2 }, { -2, 2 }, { -2, -2 }, { 0, 0 }
    }};

    static const int ND1[4] = { 0,  1,  2,  3 };
    static const int ND2[4] = { 1,  2,  3,  0 };

    static const int nn1[4] = { 2,  5,  8, 11 };
    static const int nn2[4] = { 11,  2,  5,  8 };


    int z, c;

    if (sprite.sound_count > 0)
    {
        sprite.sound_count--;
    }

    int d{ (sprite.frame - 1) / 3 };

    if (d < 4) /* turn n s e w */
    {
        z = (sprite.frame - 1) % 3;

        if (z == 2)
        {
            sprite.step = 0;
        }
        if (z == 0)
        {
            sprite.step = 1;
        }
        if (sprite.step)
        {
            z++;
        }
        else
        {
            z--;
        }

        getDirection(sprite.position.x, sprite.position.y, sprite.destination.x, sprite.destination.y);
        if (absDist < 60)
        {
            if (sprite.flag == 0)
            {
                sprite.flag = 1;
                sprite.destination = sprite.origin;
            }
            else
            {
                sprite.active = false;
                return;
            }
        }

        c = getDirection(sprite.position.x, sprite.position.y, sprite.destination.x, sprite.destination.y);
        c = (c - 1) / 2;

        if ((c != d) && (!RandomRange(0, 10)))
        {
            if (Rand16() & 1)
            {
                z = ND1[d];
            }
            else
            {
                z = ND2[d];
            }

            d = 4;
            if (!sprite.sound_count)
            {
                MakeSound("city", "Monster -speed [MonsterSpeed]");
                sprite.sound_count = 50 + RandomRange(0, 100);
            }
        }
    }
    else
    {
        d = 4;
        c = sprite.frame;
        z = (c - 13) & 3;
        if (!(Rand16() & 3))
        {
            if (Rand16() & 1)
            {
                z = nn1[z];
            }
            else
            {
                z = nn2[z];
            }
            d = (z - 1) / 3;
            z = (z - 1) % 3;
        }
    }

    sprite.frame = std::clamp(((d * 3) + z) + 1, 0, 16);
    sprite.position += G[d];

    if (sprite.count > 0)
    {
        sprite.count--;
    }

    c = getTile(sprite.position + sprite.hot);
    
    if ((c == -1) || ((c == RIVER) && (sprite.count != 0)))
    {
        sprite.active = false;
    }

    for (auto& other : Sprites)
    {
        if ((other.type == SimSprite::Type::Airplane ||
            other.type == SimSprite::Type::Helicopter ||
            other.type == SimSprite::Type::Ship ||
            other.type == SimSprite::Type::Train) &&
            spritesCollided(sprite, other))
        {
            explodeSprite(other);
        }
    }

    destroyTile(sprite.position + Vector<int>{48, 16});
}


void updateTornado(SimSprite& sprite)
{
    static int CDx[9] = { 2,  3,  2,  0, -2, -3 };
    static int CDy[9] = { -2,  0,  2,  3,  2,  0 };

    ++sprite.frame;
    if (sprite.frame >= sprite.frames.size())
    {
        sprite.frame = 0;
    }

    sprite.count--;

    if (sprite.count <= 0)
    {
        sprite.active = false;
    }

    for (auto& other : Sprites)
    {
        if ((other.type == SimSprite::Type::Airplane ||
            other.type == SimSprite::Type::Helicopter ||
            other.type == SimSprite::Type::Ship ||
            other.type == SimSprite::Type::Train) &&
            spritesCollided(sprite, other))
        {
            explodeSprite(other);
        }
    }

    const int newDirection = RandomRange(0, 5);
    sprite.position += Vector<int>{ CDx[newDirection], CDy[newDirection] };

    sprite.active = spritePositionValid(sprite);

    if ((sprite.count != 0) && RandomRange(0, 500) == 0)
    {
        sprite.active = false;
    }

    destroyTile(sprite.position + Vector<int>{48, 40});
}


void updateExplosion(SimSprite& sprite)
{
    if (sprite.frame == 0)
    {
        MakeSound("city", "Explosion-High"); // explosion
            
        int x = (sprite.position.x / 16) + 3;
        int y = (sprite.position.y / 16);
            
        SendMesAt(NotificationId::ExplosionReported, x, y);
    }

    sprite.frame++;

    if (sprite.frame > 5)
    {
        sprite.frame = 0;
        sprite.active = false;

        startFire(sprite.position + Vector<int>{48 - 8, 16});
        startFire(sprite.position + Vector<int>{48 - 24, 0});
        startFire(sprite.position + Vector<int>{48 + 8, 0});
        startFire(sprite.position + Vector<int>{48 - 24, 32});
        startFire(sprite.position + Vector<int>{48 + 8, 32});
        return;
    }
}


void updateSprites()
{
    if (Paused())
    {
        return;
    }

    Cycle++;

    for (auto& sprite : Sprites)
    {
        if (sprite.active)
        {
            switch (sprite.type)
            {
            case SimSprite::Type::Train:
                updateTrain(sprite);
                break;

            case SimSprite::Type::Helicopter:
                updateHelicopter(sprite);
                break;

            case SimSprite::Type::Airplane:
                updateAirplane(sprite);
                break;

            case SimSprite::Type::Ship:
                updateShip(sprite);
                break;

            case SimSprite::Type::Monster:
                updateMonster(sprite);
                break;

            case SimSprite::Type::Tornado:
                updateTornado(sprite);
                break;

            case SimSprite::Type::Explosion:
                updateExplosion(sprite);
                break;
            }
        }
    }
}


void generateTrain(const Point<int>& position)
{
    // What exactly does 'train groove' mean?
    constexpr Vector<int> TrainGroove{-39, 6};

    if (TotalPop > 20 && getSprite(SimSprite::Type::Train) == nullptr && RandomRange(0, 25) == 0)
    {
        makeSprite(SimSprite::Type::Train, position.skewBy({ 16, 16 }) + TrainGroove);
    }
}


void makeShipAt(const Point<int>& position)
{
    makeSprite(SimSprite::Type::Ship, position.skewBy({ 16, 16 }) - Vector<int>{ 48 - 1, 0 });
}


void generateShip()
{
    switch (RandomRange(0, 3))
    {
    case 0:
        for (int x = 4; x < SimWidth - 2; x++)
        {
            if (Map[x][0] == CHANNEL)
            {
                makeShipAt({ x, 0 });
                return;
            }
        }
        break;

    case 1:
        for (int y = 1; y < SimHeight - 2; y++)
        {
            if (Map[0][y] == CHANNEL)
            {
                makeShipAt({ 0, y });
                return;
            }
        }
        break;

    case 2:
        for (int x = 4; x < SimWidth - 2; x++)
        {
            if (Map[x][SimHeight - 2] == CHANNEL)
            {
                makeShipAt({ x, SimHeight - 2 });
                return;
            }
        }
        break;

    case 3:
        for (int y = 1; y < SimHeight - 2; y++)
        {
            if (Map[SimWidth - 2][y] == CHANNEL)
            {
                makeShipAt({ SimWidth - 2, y });
                return;
            }
        }
        break;
    }
}


void makeMonsterAt(const Point<int>& position)
{
    makeSprite(SimSprite::Type::Monster, position.skewBy({ 16,16 }) + Vector<int>{ 48, 0 });
    ClearMes();
    SendMesAt(NotificationId::MonsterReported, position.x + 5, position.y);
}


bool findSpawnPosition()
{
    for (int z = 0; z < 300; z++)
    {
        const int x = RandomRange(0, SimWidth - 20) + 10;
        const int y = RandomRange(0, SimHeight - 10) + 5;
        if ((Map[x][y] == RIVER) || (Map[x][y] == RIVER + BULLBIT))
        {
            makeMonsterAt({ x, y });
            return true;
        }
    }

    return false;
}


void generateMonster()
{
    SimSprite* sprite = getSprite(SimSprite::Type::Monster);
    if (sprite)
    {
        sprite->sound_count = 1;
        sprite->count = 1000;
        sprite->destination = { pollutionMax().x * 16, pollutionMax().y * 16 };
    }

    if (!findSpawnPosition())
    {
        makeMonsterAt({ 60, 50 });
    }
}


void generateHelicopter(const Point<int>& position)
{
    SimSprite* sprite{ getSprite(SimSprite::Type::Helicopter) };
    if (sprite != nullptr && sprite->active)
    {
        return;
    }

    makeSprite(SimSprite::Type::Helicopter, position.skewBy({ 16, 16 }) + Vector<int>{ 0, 30 });
}


void generateAirplane(const Point<int>& position)
{
    SimSprite* sprite{ getSprite(SimSprite::Type::Airplane) };
    if (sprite != nullptr && sprite->active)
    {
        return;
    }

    makeSprite(SimSprite::Type::Airplane, position.skewBy({ 16, 16 }) + Vector<int>{ 48, 12 });
}


void generateTornado()
{
    SimSprite* sprite{ getSprite(SimSprite::Type::Tornado) };

    if (sprite != nullptr)
    {
        sprite->count = 200;
        sprite->active = true;
        //return;
    }

    const Point<int> location{ RandomRange(1, SimWidth - 2), RandomRange(1, SimHeight - 2) };

    makeSprite(SimSprite::Type::Tornado, location.skewBy({ 16, 16 }));
    ClearMes();
    SendMesAt(NotificationId::TornadoReported, location.x, location.y);
}


void makeExplosionAt(const Point<int>& position)
{
    makeSprite(SimSprite::Type::Explosion, position - Vector<int>{ 40, 16 });
}


void generateExplosion(const Point<int>& position)
{
    if ((position.x >= 0) && (position.x < SimWidth) && (position.y >= 0) && (position.y < SimHeight))
    {
        makeExplosionAt(position.skewBy({ 16, 16 }) + Vector<int>{ 8, 8 });
    }
}
