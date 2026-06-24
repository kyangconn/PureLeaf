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
    T value_{};
    Error error = Error::None;

    bool ok() const { return error == Error::None; }

    T& value() { return value_; }
    const T& value() const { return value_; }

    static Result<T> Ok(T v) { return {std::move(v), Error::None}; }
    static Result<T> Err(Error e) { return {T{}, e}; }
};

/// void specialization — for operations that only succeed or fail.
template <>
struct Result<void> {
    Error error = Error::None;

    bool ok() const { return error == Error::None; }

    static Result<void> Ok() { return {Error::None}; }
    static Result<void> Err(Error e) { return {e}; }
};

}  // namespace pureleaf
