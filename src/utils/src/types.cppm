module;

#include <functional>
#include <optional>

export module utils:types;

export namespace utils {

template <typename T>
using optional_ref = std::optional<std::reference_wrapper<T>>;
template <typename T>
using optional_cref = std::optional<std::reference_wrapper<const T>>;

} // namespace utils
