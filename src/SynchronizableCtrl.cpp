/*
 * Copyright (C) 2026 The GMetronome Team
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

#include "SynchronizableCtrl.h"

namespace {

  // helper
  std::chrono::microseconds goLiveTime(const audio::Ticker::Info& info)
  { return info.timestamp + info.backend_latency; }

}//unnamed namespace

void TickerInfoQueue::push(const audio::Ticker::Info& info)
{
  // check timestamp validity
  if (!q_.empty() && info.timestamp <= q_.back().timestamp)
  {
#ifndef NDEBUG
    std::cerr << "TickerInfoQueue: failed to enqueue Ticker::Info (invalid timestamp)" << std::endl;
#endif
    return;
  }

  if (head_it_ != q_.begin())
  {
    q_.front() = info;
    q_.splice(q_.end(), q_, q_.begin());
  }
  else
  {
    q_.push_back(info);
  }

  if (head_it_ == q_.end())
    head_it_ = std::prev(q_.end());
}

std::optional<audio::Ticker::Info> TickerInfoQueue::pop(const std::chrono::microseconds& time)
{
  forwardHeadIterator(time);

  if (head_it_ != q_.end() && goLiveTime(*head_it_) <= time)
    return {*head_it_++};
  else
    return {};
}

void TickerInfoQueue::clear()
{
  q_.clear();
  head_it_ = q_.end();
}

/**
   Forward the head iterator to the next info with a timestamp and latency <= time
   or q_.end() if no such element exists.
*/
void TickerInfoQueue::forwardHeadIterator(const std::chrono::microseconds& time)
{
  if (head_it_ == q_.end())
    return;

  for(auto next_it = std::next(head_it_);
      next_it != q_.end() && goLiveTime(*next_it) <= time;
      ++next_it, ++head_it_) ;
}
