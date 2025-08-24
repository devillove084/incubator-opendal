/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace opendal::ffi::async_op {
struct RustFutureRead;
struct RustFutureWrite;
struct RustFutureList;
struct RustFutureBool;
struct RustFutureReaderId;
struct RustFutureListerId;
struct RustFutureEntryOption;
struct Operator;
}  // namespace opendal::ffi::async_op

namespace opendal::async {

class Operator {
 public:
  Operator(std::string_view scheme,
           const std::unordered_map<std::string, std::string> &config = {});

  // Disable copy and assign
  Operator(const Operator &) = delete;
  Operator &operator=(const Operator &) = delete;

  // Enable move
  Operator(Operator &&) noexcept;
  Operator &operator=(Operator &&) noexcept;
  ~Operator();

  using ReadFuture = opendal::ffi::async_op::RustFutureRead;
  auto Read(std::string_view path) -> ReadFuture;

  using WriteFuture = opendal::ffi::async_op::RustFutureWrite;
  auto Write(std::string_view path, std::span<uint8_t> data) -> WriteFuture;

  using ListFuture = opendal::ffi::async_op::RustFutureList;
  auto List(std::string_view path) -> ListFuture;

  using ExistsFuture = opendal::ffi::async_op::RustFutureBool;
  auto Exists(std::string_view path) -> ExistsFuture;

  using CreateDirFuture = opendal::ffi::async_op::RustFutureWrite;
  auto CreateDir(std::string_view path) -> CreateDirFuture;

  using CopyFuture = opendal::ffi::async_op::RustFutureWrite;
  auto Copy(std::string_view from, std::string_view to) -> CopyFuture;

  using RenameFuture = opendal::ffi::async_op::RustFutureWrite;
  auto Rename(std::string_view from, std::string_view to) -> RenameFuture;

  using DeleteFuture = opendal::ffi::async_op::RustFutureWrite;
  auto DeletePath(std::string_view path) -> DeleteFuture;

  using ReaderFuture = opendal::ffi::async_op::RustFutureReaderId;
  auto GetReader(std::string_view path) -> ReaderFuture;

  using ListerFuture = opendal::ffi::async_op::RustFutureListerId;
  auto GetLister(std::string_view path) -> ListerFuture;

 private:
  class OperatorImpl;
  std::unique_ptr<OperatorImpl> operator_impl_;
};

/**
 * @class Reader
 * @brief Async Reader is designed to read data from a specific path in an
 * asynchronous manner.
 * @details It provides streaming read operations with range support.
 */
class Reader {
 public:
  // Disable copy and assign
  Reader(const Reader &) = delete;
  Reader &operator=(const Reader &) = delete;

  // Enable move
  Reader(Reader &&other) noexcept;
  Reader &operator=(Reader &&other) noexcept;
  ~Reader() noexcept;

  // Constructor from ID (for tests and advanced usage)
  explicit Reader(size_t reader_id) noexcept;

  using ReadFuture = opendal::ffi::async_op::RustFutureRead;

  /**
   * @brief Read data from the specified range
   * @param start Start offset in bytes
   * @param len Number of bytes to read
   * @return Future that resolves to the read data
   */
  auto Read(uint64_t start, uint64_t len) -> ReadFuture;

 private:
  friend class Operator;

  void Destroy() noexcept;

  size_t reader_id_{0};
};

/**
 * @class Lister
 * @brief Async Lister is designed to list entries at a specified path in an
 * asynchronous manner.
 * @details It provides streaming iteration over directory entries.
 */
class Lister {
 public:
  // Disable copy and assign
  Lister(const Lister &) = delete;
  Lister &operator=(const Lister &) = delete;

  // Enable move
  Lister(Lister &&other) noexcept;
  Lister &operator=(Lister &&other) noexcept;
  ~Lister() noexcept;

  // Constructor from ID (for tests and advanced usage)
  explicit Lister(size_t lister_id) noexcept;

  using NextFuture = opendal::ffi::async_op::RustFutureEntryOption;

  /**
   * @brief Get the next entry in the listing
   * @return Future that resolves to the next entry path, or empty string if no
   * more entries
   */
  auto Next() -> NextFuture;

 private:
  friend class Operator;

  void Destroy() noexcept;

  size_t lister_id_{0};
};

}  // namespace opendal::async
