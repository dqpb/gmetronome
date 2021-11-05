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

#ifndef GMetronome_Error_h
#define GMetronome_Error_h

#include <exception>
#include <string>

enum class MessageCategory
{
  kInformation = 0,
  kWarning = 1,
  kError = 2
};

struct Message
{
  const MessageCategory category;
  const std::string text;
};

class GMetronomeError : public std::exception {
public:
  GMetronomeError(const std::string& text = "",
                  MessageCategory category = MessageCategory::kWarning);

  const Message& message() const noexcept
    { return message_; }
  
  const char* what() const noexcept override
    { return message_.text.data(); }
  
  const std::string& text() const noexcept
    { return message_.text; }
  
  MessageCategory category() const noexcept
    { return message_.category; }
  
private:
  Message message_;
};

#endif//GMetronome_Error_h
