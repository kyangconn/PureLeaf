#pragma once

#include <utility>

namespace pureleaf {

/// Error codes used across core services.
enum class Error {
    None = 0,
    NotFound,
    AlreadyExists,
    InvalidPath,
    IoError,
    InvalidArgument,
    Internal,
};

/// Simple Result wrapper. T must be default-constructible.
template <typename T>
struct Result {
    T value{};
    Error error = Error::None;

    bool ok() const { return error == Error::None; }

    static Result<T> Ok(T v) { return {std::move(v), Error::None}; }
    static Result<T> Err(Error e) { return {T{}, e}; }
};

}  // namespace pureleaf
