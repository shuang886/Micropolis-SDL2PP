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
#pragma once

#include "main.h"

#include "Point.h"
#include "Vector.h"

#include <array>

extern std::array<std::array<int, SimHeight>, SimWidth> Map; // Main Map 120 x 100

void DrawBigMapSegment(const Point<int>& begin, const Point<int>& end);
void DrawBigMap();

unsigned int tileValue(const int x, const int y);
unsigned int maskedTileValue(const int x, const int y);
unsigned int maskedTileValue(unsigned int tile);

bool tilePowered(const unsigned int tile);
bool tileIsZoned(const unsigned int tile);
bool blink();
void ResetMap();
