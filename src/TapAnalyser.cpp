/*
 * Copyright (C) 2021 The GMetronome Team
 * 
 * This file is part of GMetronome.
 *
 * GMetronome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GMetronome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMetronome.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TapAnalyser.h"
#include <algorithm>
#include <iostream>

using std::chrono::microseconds;  
using std::chrono::milliseconds;  
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;

const milliseconds kSlotDuration = 10ms;

template<typename T>
void print(const T& container)
{
  // for (auto& t : container)
  //   std::cout << t.slot << "[" << t.value << "] ";

  // std::cout << std::endl;
}

TapAnalyser::TapAnalyser()
{
  taps_.reserve(200);
  corr_.reserve(200);
}

void TapAnalyser::tap()
{
  auto now = std::chrono::steady_clock::now();

  if ( taps_.empty() )
  {
    tap_start_ = now;
    taps_.push_back({0,1.});
  }
  else
  {
    milliseconds delay =
      std::chrono::duration_cast<milliseconds>(now - tap_start_);
    
    int tap_slot = delay.count() / kSlotDuration.count();
    
    // gaussian bell
    //taps_.push_back({ tap_slot - 4 , 0.000133830226 });
    taps_.push_back({ tap_slot - 3 , 0.004431848412 });
    taps_.push_back({ tap_slot - 2 , 0.053990966513 });
    taps_.push_back({ tap_slot - 1 , 0.241970724519 });
    taps_.push_back({ tap_slot     , 0.398942280401 });
    taps_.push_back({ tap_slot + 1 , 0.241970724519 });
    taps_.push_back({ tap_slot + 2 , 0.053990966513 });
    taps_.push_back({ tap_slot + 3 , 0.004431848412 });
    //taps_.push_back({ tap_slot + 4 , 0.000133830226 });
  }

  correlate();
}

void TapAnalyser::reset()
{
  taps_.clear();
}

Estimate<double> TapAnalyser::tempo()
{
  // not implemented yet
  return {};
}

Estimate<Meter> TapAnalyser::meter()
{
  // not implemented yet
  return {};
}

double TapAnalyser::beat()
{
  // not implemented yet
  return {};
}

void TapAnalyser::correlate()
{
  auto comp_tap_int = [] (const Tap_& a, const int& k) -> bool
    { return a.slot < k; };

  corr_.clear();
  
  int n = taps_.back().slot;
  auto start_rhs = taps_.begin();
  
  for (int k = 0; k < n; ++k)
  {
    double value = 0;
    
    if (k > start_rhs->slot)
      ++start_rhs;
    
    auto lhs = taps_.begin();
    auto rhs = start_rhs;

    if (rhs == taps_.end())
      break;
    
    for (; lhs != taps_.end(); ++lhs)
    {
      rhs = std::lower_bound(rhs, taps_.end(), lhs->slot + k, comp_tap_int);
      
      if (rhs == taps_.end())
        break;
      
      if ((rhs->slot - lhs->slot) == k)
        value += (lhs->value * rhs->value);
    }
    
    if (value != 0)
      corr_.push_back({ k, value });
  }

  std::cout << "TAPS: ";
  print(taps_);
  std::cout << "CORR: ";
  print(corr_);

  auto comp_tap_tap = [] (const Tap_& a, const Tap_& b) -> bool
    { return a.value > b.value; };

  if (corr_.size() > 5)
  {
    std::sort(corr_.begin(), corr_.end(), comp_tap_tap);

    std::cout << "CAND: ";
    int i=0;
    for (auto& cand : corr_)
    {
      double tempo = 1. / (cand.slot * kSlotDuration.count() / 1000. / 60.);
//      if (tempo>0 && tempo<250)
      {
        std::cout << cand.slot << "[" << cand.value << "] "
                  << tempo
                  << " | ";
      }
      
      if (++i==20)
        break;
    }
    std::cout << std::endl ;
  }
  
  std::cout << std::endl ;
}
