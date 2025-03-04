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
#include "w_update.h"

#include "Budget.h"

#include "main.h"

#include "s_msg.h"

#include "Scan.h"

#include "w_sound.h"
#include "w_tk.h"
#include "w_util.h"


#include <algorithm>
#include <limits>
#include <string>


namespace
{
    int lastCityTime{};
    int lastCityYear{};
    int lastCityMonth{};

    bool NewMonth{ false };

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


bool newMonth()
{
    return NewMonth;
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
    constexpr auto megaannum = 1000000; // wierd place for this

    lastCityTime = CityTime / 4;

    int year = (CityTime / 48) + StartingYear;
    int month = (CityTime % 48) / 4;

    if (year >= megaannum)
    {
        SetYear(StartingYear);
        year = StartingYear;
        SendMes(NotificationId::BrownoutsReported);
    }

    doMessage();

    NewMonth = false;
    if ((LastCityYear() != year) || (LastCityMonth() != month))
    {
        lastCityYear = year;
        lastCityMonth = month;

        NewMonth = true;

        if (month == 0 && !autoBudget() && !newMap())
        {
            showBudgetWindow();
        }
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


void UpdateFunds(Budget& budget)
{
    budget.PreviousFunds(budget.CurrentFunds());
    budget.CurrentFunds(std::clamp(budget.CurrentFunds(), 0, std::numeric_limits<int>::max()));
}
