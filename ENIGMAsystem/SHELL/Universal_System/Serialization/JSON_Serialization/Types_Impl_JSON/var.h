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

#ifndef ENIGMA_SERIALIZE_VAR_JSON_H
#define ENIGMA_SERIALIZE_VAR_JSON_H

#include "../../../var4.h"
#include "../../type_traits.h"
#include "lua_table.h"
#include "variant.h"

namespace enigma {
namespace JSON_serialization {

template <typename T>
is_t<T, var, std::string> internal_serialize_into_fn(const T &value) {
  std::string json = "{\"variant\":";

  json += internal_serialize_into_fn<variant>(value);
  json += ",\"array1d\":";
  json += internal_serialize_into_fn(value.array1d);
  json += ",\"array2d\":";
  json += internal_serialize_into_fn(value.array2d);

  json += "}";
  return json;
}

template <typename T>
is_t<T, var, T> inline internal_deserialize_fn(const std::string &json) {
  std::string variantStart = "\"variant\":";
  std::string array1dStart = "\"array1d\":";
  std::string array2dStart = "\"array2d\":";

  std::size_t variantPos = json.find(variantStart);
  std::size_t array1dPos = json.find(array1dStart);
  std::size_t array2dPos = json.find(array2dStart);

  std::string variantStr =
      json.substr(variantPos + variantStart.length(), array1dPos - variantPos - variantStart.length() - 1);
  std::string array1dStr =
      json.substr(array1dPos + array1dStart.length(), array2dPos - array1dPos - array1dStart.length() - 1);
  std::string array2dStr =
      json.substr(array2dPos + array2dStart.length(), json.length() - array2dPos - array2dStart.length() - 1);

  variant inner = internal_deserialize_fn<variant>(variantStr);
  var result{std::move(inner)};

  result.array1d = internal_deserialize_fn<lua_table<variant>>(array1dStr);
  result.array2d = internal_deserialize_fn<lua_table<lua_table<variant>>>(array2dStr);

  return result;
}

}  // namespace JSON_serialization
}  // namespace enigma

#endif
