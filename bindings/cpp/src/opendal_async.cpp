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

#include "opendal_async.hpp"

#include <iterator>

#include "async.rs.h"

#define RUST_STR(s) rust::Str(s.data(), s.size())
#define RUST_STRING(s) rust::String(s.data(), s.size())

using namespace opendal::async;

static rust::Box<opendal::ffi::async_op::Operator> new_operator(
    std::string_view scheme,
    const std::unordered_map<std::string, std::string> &config) {
  auto rust_map = rust::Vec<opendal::ffi::async_op::HashMapValue>();
  rust_map.reserve(config.size());
  for (auto &[k, v] : config) {
    rust_map.push_back({RUST_STRING(k), RUST_STRING(v)});
  }

  return opendal::ffi::async_op::new_operator(RUST_STR(scheme), rust_map);
}

class Operator::OperatorImpl {
 public:
  OperatorImpl(std::string_view scheme,
               const std::unordered_map<std::string, std::string> &config)
      : operator_(new_operator(scheme, config)) {}
  ~OperatorImpl() = default;

  opendal::ffi::async_op::Operator *GetOperator() { return &*operator_; }

 private:
  rust::Box<opendal::ffi::async_op::Operator> operator_;
};

Operator::Operator(std::string_view scheme,
                   const std::unordered_map<std::string, std::string> &config)
    : operator_impl_(
          std::make_unique<OperatorImpl>(std::move(scheme), config)) {}

Operator::Operator(Operator &&other) noexcept = default;
Operator &Operator::operator=(Operator &&other) noexcept = default;
Operator::~Operator() = default;

auto Operator::Read(std::string_view path) -> Operator::ReadFuture {
  return opendal::ffi::async_op::operator_read(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::Write(std::string_view path, std::span<uint8_t> data)
    -> Operator::WriteFuture {
  rust::Vec<uint8_t> vec;
  std::copy(data.begin(), data.end(), std::back_inserter(vec));

  return opendal::ffi::async_op::operator_write(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path), vec);
}

auto Operator::List(std::string_view path) -> Operator::ListFuture {
  return opendal::ffi::async_op::operator_list(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::Exists(std::string_view path) -> Operator::ExistsFuture {
  return opendal::ffi::async_op::operator_exists(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::CreateDir(std::string_view path) -> Operator::CreateDirFuture {
  return opendal::ffi::async_op::operator_create_dir(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::Copy(std::string_view from, std::string_view to)
    -> Operator::CopyFuture {
  return opendal::ffi::async_op::operator_copy(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(from), RUST_STRING(to));
}

auto Operator::Rename(std::string_view from, std::string_view to)
    -> Operator::RenameFuture {
  return opendal::ffi::async_op::operator_rename(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(from), RUST_STRING(to));
}

auto Operator::DeletePath(std::string_view path) -> Operator::DeleteFuture {
  return opendal::ffi::async_op::operator_delete(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::GetReader(std::string_view path) -> Operator::ReaderFuture {
  return opendal::ffi::async_op::operator_reader(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

auto Operator::GetLister(std::string_view path) -> Operator::ListerFuture {
  return opendal::ffi::async_op::operator_lister(
      opendal::ffi::async_op::OperatorPtr{operator_impl_->GetOperator()},
      RUST_STRING(path));
}

// Reader implementation
Reader::Reader(size_t reader_id) noexcept : reader_id_(reader_id) {}

Reader::Reader(Reader &&other) noexcept : reader_id_(other.reader_id_) {
  other.reader_id_ = 0;
}

Reader &Reader::operator=(Reader &&other) noexcept {
  if (this != &other) {
    reader_id_ = other.reader_id_;
    other.reader_id_ = 0;
  }
  return *this;
}

Reader::~Reader() noexcept { Destroy(); }

void Reader::Destroy() noexcept {
  if (reader_id_ != 0) {
    opendal::ffi::async_op::delete_reader(
        opendal::ffi::async_op::ReaderPtr{reader_id_});
    reader_id_ = 0;
  }
}

auto Reader::Read(uint64_t start, uint64_t len) -> Reader::ReadFuture {
  return opendal::ffi::async_op::reader_read(
      opendal::ffi::async_op::ReaderPtr{reader_id_}, start, len);
}

// Lister implementation
Lister::Lister(size_t lister_id) noexcept : lister_id_(lister_id) {}

Lister::Lister(Lister &&other) noexcept : lister_id_(other.lister_id_) {
  other.lister_id_ = 0;
}

Lister &Lister::operator=(Lister &&other) noexcept {
  if (this != &other) {
    lister_id_ = other.lister_id_;
    other.lister_id_ = 0;
  }
  return *this;
}

Lister::~Lister() noexcept { Destroy(); }

void Lister::Destroy() noexcept {
  if (lister_id_ != 0) {
    opendal::ffi::async_op::delete_lister(
        opendal::ffi::async_op::ListerPtr{lister_id_});
    lister_id_ = 0;
  }
}

auto Lister::Next() -> Lister::NextFuture {
  return opendal::ffi::async_op::lister_next(
      opendal::ffi::async_op::ListerPtr{lister_id_});
}
