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
#include <set>
#include <iostream>
#include <cmath>
#include <optional>

using std::chrono::microseconds;  
using std::chrono::milliseconds;  
using std::chrono::seconds;  
using std::literals::chrono_literals::operator""ms;
using std::literals::chrono_literals::operator""us;
using std::literals::chrono_literals::operator""s;

const seconds kTapTimeout = 2s;
const std::size_t kMaxTaps = 20;
const milliseconds kTimeSlotDuration = 5ms;

TapAnalyser::TapAnalyser()
{}

void TapAnalyser::tap(double value)
{
  value = std::clamp(value, 0., 1.);
  
  if (value == 0)
    return;
  
  auto now = std::chrono::steady_clock::now();

  if ( ! taps_.empty() && (now - taps_.back().time) > kTapTimeout )
    taps_.clear();
  
  taps_.push_back({ now, value });
  
  if (taps_.size() > kMaxTaps)
    taps_.pop_front();
    
  correlate();

  /*
  dbscan();

  if (corr_.size() < 2)
    return;
 
  auto it = std::max_element(corr_.begin(), corr_.end(),
                             [] (const auto& lhs, const auto& rhs) -> bool
                               { return lhs.label < rhs.label; });

  int N = it->label + 1;
  std::cout << "Number of clusters: " << N << std::endl;
  
  auto d = std::count_if(corr_.begin(), corr_.end(),
                         [] (const auto& a) -> bool
                           { return a.label == kNoise; });
  
  std::cout << "Noise points: " << d << std::endl;

  std::vector<int>    cluster_count(N,0);
  std::vector<double> cluster_value_sum(N,0);
  std::vector<double> cluster_max_value(N,0);
  std::vector<double> cluster_max_value_slot(N,0);
  std::vector<double> cluster_tempo(N,0);
  
  for (auto& a : corr_)
  {
    if (a.label >= 0)
    {
      cluster_count[a.label] += 1;
      cluster_value_sum[a.label] += a.value;
      if (a.value > cluster_max_value[a.label])
      {
        cluster_max_value[a.label] = a.value;
        cluster_max_value_slot[a.label] = a.time_slot;
      }
    }
  }

  for (int c=0; c<N; ++c)
  {
    cluster_tempo[c] = 1. / (cluster_max_value_slot[c] * kTimeSlotDuration.count() / 1000. / 60.);
    std::cout << "Cluster:" << c
              << " Value-Sum:" << cluster_value_sum[c]
              << " Tempo:" << cluster_tempo[c] << std::endl; 
  }
  */
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
  auto comp_tap_int = [] (const DataPoint_& a, const int& k) -> bool
    { return a.time_slot < k; };

  std::vector<DataPoint_> corr_in_;
  corr_in_.reserve(taps_.size() * 7);
  
  corr_.clear();
  corr_.reserve(corr_in_.size());

  auto tap_start = taps_.front().time;
    
  for (const auto& t : taps_)
  {
    milliseconds delay =
      std::chrono::duration_cast<milliseconds>(t.time - tap_start);
    
    int time_slot = delay.count() / kTimeSlotDuration.count();
    
    // unnormalized gaussian bell
    corr_in_.push_back({ time_slot - 4 , t.value * 0.000335462628 , 0});
    corr_in_.push_back({ time_slot - 3, t.value * 0.011108996538, 0 });
    corr_in_.push_back({ time_slot - 2, t.value * 0.135335283237, 0 });
    corr_in_.push_back({ time_slot - 1, t.value * 0.606530659713, 0 });
    corr_in_.push_back({ time_slot, t.value * 1, 0 });
    corr_in_.push_back({ time_slot + 1, t.value * 0.606530659713, 0 });
    corr_in_.push_back({ time_slot + 2, t.value * 0.135335283237, 0 });
    corr_in_.push_back({ time_slot + 3, t.value * 0.011108996538, 0 });
    corr_in_.push_back({ time_slot + 4, t.value * 0.000335462628 , 0 });
    
  }
  
  int n = corr_in_.back().time_slot;
  auto start_rhs = corr_in_.begin();
  
  for (int k = corr_in_.front().time_slot; k < n; ++k)
  {
    double value = 0;
    
    if (k > start_rhs->time_slot)
      ++start_rhs;
    
    auto lhs = corr_in_.begin();
    auto rhs = start_rhs;

    if (rhs == corr_in_.end())
      break;
    
    for (; lhs != corr_in_.end(); ++lhs)
    {
      rhs = std::lower_bound(rhs, corr_in_.end(), lhs->time_slot + k, comp_tap_int);
      
      if (rhs == corr_in_.end())
        break;
      
      if ((rhs->time_slot - lhs->time_slot) == k)
        value += (lhs->value * rhs->value);
    }
    
    if (k > 20 && value != 0)
    {
      // apply Rayleigh weighting function
      double normalized_value = value / n;

      static const double center_tempo = 120.; // bpm
      static const double center_slot = (1000. / (center_tempo / 60.)) / kTimeSlotDuration.count();
      static const double theta = 2.0;
      
      double weight =  std::exp( -0.5 * ( std::pow ( std::log2( k / center_slot ), 2 ) / theta  ) );
      
      corr_.push_back(
        {
          k,
          normalized_value,
          normalized_value * weight
        });
    }
  }

  if (corr_.size() > 2)
  {
    auto pt = corr_.begin() + 2;
    do {
      if ( (pt-2)->value < (pt-1)->value && (pt-1)->value > (pt)->value )
        std::cout << "Peak at " << (pt-1)->time_slot
                  << " \tvalue: " << (pt-1)->value 
                  << " \tweighted_value: " << (pt-1)->weighted_value << std::endl;
      
    } while ( ++pt != corr_.end() );
  }


  auto comp_tap_tap = [] (const DataPoint_& a, const DataPoint_& b) -> bool
    { return a.weighted_value > b.weighted_value; };
  
  if (corr_.size() > 5)
  {
    std::sort(corr_.begin(), corr_.end(), comp_tap_tap);

    std::cout << "CAND: ";
    int i=0;
    for (auto& cand : corr_)
    {
      double tempo = 1. / (cand.time_slot * kTimeSlotDuration.count() / 1000. / 60.);
//      if (tempo>0 && tempo<250)
      {
        std::cout << cand.time_slot << "[" << cand.value << ", " << cand.weighted_value << "] "
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


/*
void TapAnalyser::dbscan()
{
  auto rangeQuery = [&] (std::size_t Q_index) -> std::set<std::size_t>
    {
      const auto& Q = corr_[Q_index];
      std::set<std::size_t> N;
      for ( std::size_t P_index = 0; P_index < corr_.size(); ++P_index ) {
        const auto& P = corr_[P_index];
        if ( std::abs(Q.time_slot - P.time_slot) < 3 )
          N.insert(P_index);
      }
      return N;
    };
  
  int C = 0;
  std::size_t minPts = 5;
  
  for ( std::size_t P_index = 0; P_index < corr_.size(); ++P_index )
  {
    auto& P = corr_[P_index];
    
    if (P.label != kUndefined)
      continue;
    
    auto N_P = rangeQuery(P_index);

    std::cout << "Neighbors: " << N_P.size() << std::endl;
    
    if (N_P.size() < minPts)
    {
      P.label = kNoise;
      continue;
    }
    
    P.label = C;
    C = C + 1;
    
    N_P.erase(P_index);
    
    while ( ! N_P.empty() )
    {
      std::size_t Q_index = *N_P.begin();
      auto& Q = corr_[Q_index];
      N_P.erase ( N_P.begin() );
      
      if (Q.label == kNoise)
        Q.label = C;

      if (Q.label != kUndefined)
        continue;

      Q.label = C;

      auto N_Q = rangeQuery(Q_index);

      if (N_Q.size() >= minPts)
        N_P.insert(N_Q.begin(), N_Q.end());
    }
  }
}
*/
