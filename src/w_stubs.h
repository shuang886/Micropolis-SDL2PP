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

extern int autoGo;

void Spend(int dollars);
void SetFunds(int dollars);
int TotalFunds();
int LastFunds();
void LastFunds(const int funds);

int TickCount();

void InitGame();
void GameStarted();
void DoPlayNewCity();

void CityName(const std::string& name);
const std::string& CityName();

void GameLevel(const int level);
int GameLevel();
