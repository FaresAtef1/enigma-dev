/** Copyright (C) 2023 Fares Atef
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

/**
  @file JSON_parsing_utilities.h
  @brief This file contains the implementation of some utilities used in parsing JSON, 
  such as splitting a string into parts and finding a value in a JSON string.
*/

#ifndef ENIGMA_JSON_PARSING_UTILITIES_H
#define ENIGMA_JSON_PARSING_UTILITIES_H

#include "../type_traits.h"

namespace enigma {
namespace JSON_serialization {

std::vector<std::string> inline json_split(const std::string &s, char delimiter) {
  std::vector<std::string> parts;
  size_t start = 0;
  size_t end = 0;
  bool within_quotes = false;
  bool within_array = false;
  size_t num_open_braces = 0;
  size_t len = s.length();

  while (end < len) {
    if (s[end] == '"' && (end == 0 || s[end - 1] != '\\'))
      within_quotes = !within_quotes;
    else if (s[end] == '[' && !within_quotes)
      within_array = true;
    else if (s[end] == ']' && !within_quotes)
      within_array = false;
    else if (s[end] == '{' && !within_quotes)
      num_open_braces++;
    else if (s[end] == '}' && !within_quotes)
      num_open_braces--;
    else if (s[end] == delimiter && !within_quotes && !within_array && num_open_braces == 0) {
      if (end != start) parts.push_back(s.substr(start, end - start));
      start = end + 1;
    }
    end++;
  }
  
  if (start != len) parts.push_back(s.substr(start));
  return parts;
}

std::string inline json_find_value(const std::string &json, const std::string &key) {
  size_t startPos = json.find("\"" + key + "\":");
  if (startPos != std::string::npos) {
    startPos += key.length() + 3;  // Add 3 to account for quotes and colon
    size_t endPos = json.find_first_of(",}", startPos);
    if (endPos != std::string::npos) {
      return json.substr(startPos, endPos - startPos);
    }
  }
  return "";
}

}  // namespace JSON_serialization
}  // namespace enigma

#endif
