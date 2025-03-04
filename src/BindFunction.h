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
#pragma once

#include <functional>

template <class T, class U>
void BindFuncPtr(T& arg1, U arg2)
{
    auto it = std::find(arg1.begin(), arg1.end(), arg2);
    if (it != arg1.end()) { throw std::runtime_error("BindFuncPtr(): function pointer already bound."); }

    arg1.push_back(arg2);
}


template <class T, class U>
void UnbindFuncPtr(T& arg1, U arg2)
{
    auto it = std::find(arg1.begin(), arg1.end(), arg2);
    if (it == arg1.end()) { throw std::runtime_error("UnbindFuncPtr(): function pointer not bound."); }

    arg1.erase(it);
}
