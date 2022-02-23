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
#include "w_update.h"

#include "main.h"

#include "s_msg.h"

#include "w_eval.h"
#include "w_graph.h"
#include "w_sound.h"
#include "w_stubs.h"
#include "w_tk.h"
#include "w_util.h"


#include <algorithm>
#include <limits>
#include <string>


int MustUpdateOptions;


namespace
{
    int lastCityTime{};
    int lastCityYear{};
    int lastCityMonth{};

    const std::string MonthTable[12] =
    {
      "Jan",
      "Feb",
      "Mar",
      "Apr",
      "May",
      "Jun",
      "Jul",
      "Aug",
      "Sep",
      "Oct",
      "Nov",
      "Dec"
    };
}


const std::string& MonthString(Month month)
{
    return MonthTable[static_cast<int>(month)];
}


int LastCityTime()
{
    return lastCityTime;
}


void LastCityTime(int tick)
{
    lastCityTime = tick;
}


int LastCityMonth()
{
    return lastCityMonth;
}


void LastCityMonth(int month)
{
    lastCityMonth = month;
}


int LastCityYear()
{
    return lastCityYear;
}


void LastCityYear(int year)
{
    lastCityYear = year;
}


void updateDate()
{
    static int megalinium = 1000000;

    lastCityTime = CityTime / 4;

    int y = (CityTime / 48) + StartingYear;
    int m = (CityTime % 48) / 4;

    if (y >= megalinium)
    {
        SetYear(StartingYear);
        y = StartingYear;
        SendMes(NotificationId::BrownoutsReported);
    }

    doMessage();

    if ((LastCityYear() != y) || (LastCityMonth() != m))
    {
        lastCityYear = y;
        lastCityMonth = m;
    }
}


void UpdateOptionsMenu(int options)
{
  /*
  char buf[256];
  sprintf(buf, "UISetOptions %d %d %d %d %d %d %d %d",
	  (options&1)?1:0, (options&2)?1:0,
	  (options&4)?1:0, (options&8)?1:0,
	  (options&16)?1:0, (options&32)?1:0,
	  (options&64)?1:0, (options&128)?1:0);
  Eval(buf);
  */
}


void updateOptions()
{
  int options;

  if (MustUpdateOptions)
  {
    options = 0;
    if (autoBudget)
    {
        options |= 1;
    }
    if (autoGo)
    {
        options |= 2;
    }
    if (autoBulldoze)
    {
        options |= 4;
    }
    if (!NoDisasters)
    {
        options |= 8;
    }
    if (userSoundOn())
    {
        options |= 16;
    }
    if (DoAnimation)
    {
        options |= 32;
    }
    if (DoMessages)
    {
        options |= 64;
    }
    if (DoNotices)
    {
        options |= 128;
    }

    MustUpdateOptions = 0;
    UpdateOptionsMenu(options);
  }
}


void UpdateMaps()
{
  InvalidateMaps();
}


void UpdateGraphs()
{
  ChangeCensus();
}


void UpdateEvaluation()
{
  ChangeEval();
}


void UpdateHeads()
{
    lastCityTime = lastCityYear = lastCityMonth = -999999;
    sim_update_editors();
}


void UpdateFunds()
{
    SetFunds(std::clamp(TotalFunds(), 0, std::numeric_limits<int>::max()));
    LastFunds(TotalFunds());
}
