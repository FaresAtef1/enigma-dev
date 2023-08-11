#include <algorithm>
#include <cstring>
#include "serialization_fwd_decl.h"

template <typename T>
typename std::enable_if<std::is_same_v<std::string, std::decay_t<T>>>::type inline enigma::internal_serialize_into_fn(
    std::byte *iter, T &&value) {
  internal_serialize_into<std::size_t>(iter, value.size());
  std::transform(value.begin(), value.end(), iter + sizeof(std::size_t),
                 [](char c) { return static_cast<std::byte>(c); });
}

inline auto enigma::internal_serialize_fn(std::string &&value) {
  std::vector<std::byte> result;
  result.resize(sizeof(std::size_t) + value.size());
  internal_serialize_into<std::size_t>(result.data(), value.size());
  std::transform(value.begin(), value.end(), result.data() + sizeof(std::size_t),
                 [](char c) { return static_cast<std::byte>(c); });
  return result;
}

template <typename T>
typename std::enable_if<std::is_same_v<std::string, std::decay_t<T>>, T>::type inline enigma::internal_deserialize_fn(
    std::byte *iter) {
  std::size_t len = internal_deserialize_numeric<std::size_t>(iter);
  std::size_t offset = sizeof(std::size_t);
  std::string result{reinterpret_cast<char *>(iter + offset), reinterpret_cast<char *>(iter + offset + len)};
  return result;
}

template <typename T>
typename std::enable_if<std::is_same_v<std::string, std::decay_t<T>>>::type inline enigma::
    internal_resize_buffer_for_fn(std::vector<std::byte> &buffer, T &&value) {
  buffer.resize(buffer.size() + value.size() + sizeof(std::size_t));
}

template <typename T>
typename std::enable_if<std::is_same_v<std::string, std::decay_t<T>>>::type inline enigma::
    enigma_internal_deserialize_fn(T &value, std::byte *iter, std::size_t &len) {
  value = enigma::internal_deserialize<std::string>(iter + len);
  len += value.length() + sizeof(std::size_t);
}