#include "serialization_fwd_decl.h"

template <typename T>
typename std::enable_if<is_std_vector_v<std::decay_t<T>> ||
                        is_std_set_v<std::decay_t<T>>>::type inline enigma::internal_serialize_into_fn(std::byte *iter,
                                                                                                       T &&value) {
  internal_serialize_into<std::size_t>(iter, value.size());
  iter += sizeof(std::size_t);
  for (const auto &element : value) {
    internal_serialize_into(iter, element);
    iter += enigma_internal_sizeof(element);
  }
}

template <typename T>
typename std::enable_if<is_std_vector_v<std::decay_t<T>> || is_std_set_v<std::decay_t<T>>,
                        std::vector<std::byte>>::type inline enigma::internal_serialize_fn(T &&value) {
  std::vector<std::byte> result;
  result.resize(sizeof(std::size_t) + value.size() * ((value.size()) ? enigma_internal_sizeof(*value.begin()) : 0));
  internal_serialize_into<std::size_t>(result.data(), value.size());

  auto dataPtr = result.data() + sizeof(std::size_t);
  for (const auto &element : value) {
    internal_serialize_into(dataPtr, element);
    dataPtr += enigma_internal_sizeof(element);
  }
  return result;
}

template <typename T>
typename std::enable_if<is_std_vector_v<std::decay_t<T>> || is_std_set_v<std::decay_t<T>>,
                        T>::type inline enigma::internal_deserialize_fn(std::byte *iter) {
  std::size_t size = internal_deserialize_numeric<std::size_t>(iter);
  std::size_t offset = sizeof(std::size_t);

  using InnerType = typename T::value_type;
  std::vector<InnerType> result;
  result.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    InnerType element = internal_deserialize<InnerType>(iter + offset);
    insert_back(result, std::move(element));
    offset += enigma_internal_sizeof(element);
  }
  return result;
}

template <typename T>
typename std::enable_if<is_std_vector_v<std::decay_t<T>> || is_std_set_v<std::decay_t<T>>>::type inline enigma::
    internal_resize_buffer_for_fn(std::vector<std::byte> &buffer, T &&value) {
  buffer.resize(buffer.size() + value.size() * ((value.size()) ? enigma_internal_sizeof(*value.begin()) : 0) +
                sizeof(std::size_t));
}

template <typename T>
typename std::enable_if<is_std_vector_v<std::decay_t<T>> || is_std_set_v<std::decay_t<T>>>::type inline enigma::
    enigma_internal_deserialize_fn(T &value, std::byte *iter, std::size_t &len) {
  std::size_t size = enigma::internal_deserialize_numeric<std::size_t>(iter + len);
  len += sizeof(std::size_t);
  value.clear();
  using InnerType = typename T::value_type;

  for (std::size_t i = 0; i < size; ++i) {
    InnerType element = enigma::internal_deserialize<InnerType>(iter + len);
    insert_back(value, std::move(element));
    len += enigma_internal_sizeof(element);
  }
}