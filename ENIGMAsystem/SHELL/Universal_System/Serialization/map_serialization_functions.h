#include "serialization_fwd_decl.h"

template <typename T>
typename std::enable_if<is_std_map_v<std::decay_t<T>>>::type inline enigma::internal_serialize_into_fn(std::byte *iter,
                                                                                                       T &&value) {
  internal_serialize_into<std::size_t>(iter, value.size());
  iter += sizeof(std::size_t);
  for (const auto &element : value) {
    internal_serialize_into(iter, element.first);
    iter += enigma_internal_sizeof(element.first);
    internal_serialize_into(iter, element.second);
    iter += enigma_internal_sizeof(element.second);
  }
}

template <typename T>
typename std::enable_if<is_std_map_v<std::decay_t<T>>,
                        std::vector<std::byte>>::type inline enigma::internal_serialize_fn(T &&value) {
  std::vector<std::byte> result;
  result.resize(sizeof(std::size_t) + value.size() * ((value.size()) ? (enigma_internal_sizeof(value.begin()->first) +
                                                                        enigma_internal_sizeof(value.begin()->second))
                                                                     : 0));
  internal_serialize_into<std::size_t>(result.data(), value.size());

  auto dataPtr = result.data() + sizeof(std::size_t);
  for (const auto &element : value) {
    internal_serialize_into(dataPtr, element.first);
    internal_serialize_into(dataPtr, element.second);
    dataPtr += enigma_internal_sizeof(element.first) + enigma_internal_sizeof(element.second);
  }
  return result;
}

template <typename T>
typename std::enable_if<is_std_map_v<std::decay_t<T>>, T>::type inline enigma::internal_deserialize_fn(
    std::byte *iter) {
  std::size_t size = internal_deserialize_numeric<std::size_t>(iter);
  std::size_t offset = sizeof(std::size_t);

  using KeyType = typename std::decay_t<T>::key_type;
  using ValueType = typename std::decay_t<T>::mapped_type;
  std::map<KeyType, ValueType> result;
  result.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    KeyType key = internal_deserialize<KeyType>(iter + offset);
    offset += enigma_internal_sizeof(key);
    ValueType val = internal_deserialize<ValueType>(iter + offset);
    offset += enigma_internal_sizeof(val);
    result.insert(std::move(std::pair<KeyType, ValueType>{key, val}));
  }
  return result;
}

template <typename T>
typename std::enable_if<is_std_map_v<std::decay_t<T>>>::type inline enigma::internal_resize_buffer_for_fn(
    std::vector<std::byte> &buffer, T &&value) {
  buffer.resize(buffer.size() +
                value.size() * ((value.size()) ? (enigma_internal_sizeof(value.begin()->first) +
                                                  enigma_internal_sizeof(value.begin()->second))
                                               : 0) +
                sizeof(std::size_t));
}

template <typename T>
typename std::enable_if<is_std_map_v<std::decay_t<T>>>::type inline enigma::enigma_internal_deserialize_fn(
    T &value, std::byte *iter, std::size_t &len) {
  std::size_t size = enigma::internal_deserialize_numeric<std::size_t>(iter + len);
  len += sizeof(std::size_t);
  value.clear();
  using KeyType = typename std::decay_t<T>::key_type;
  using ValueType = typename std::decay_t<T>::mapped_type;

  for (std::size_t i = 0; i < size; ++i) {
    KeyType key = enigma::internal_deserialize<KeyType>(iter + len);
    len += enigma_internal_sizeof(key);
    ValueType val = enigma::internal_deserialize<ValueType>(iter + len);
    len += enigma_internal_sizeof(val);
    value.insert(std::move(std::pair<KeyType, ValueType>{key, val}));
  }
}