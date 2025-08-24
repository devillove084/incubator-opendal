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

#include "lib.rs.h"
#include "opendal.hpp"
#include "opendal_type.hpp"
#include "utils/ffi_converter.hpp"

namespace opendal {

class Lister::ListerImpl {
 public:
  ListerImpl(ffi::Lister *lister) : lister_(lister) {}

  ~ListerImpl() {
    if (lister_) {
      ffi::delete_lister(lister_);
      lister_ = nullptr;
    }
  }

  std::optional<Entry> next() {
    auto rust_entry = lister_->next();
    if (!rust_entry.has_value) {
      return std::nullopt;
    }
    return utils::parse_entry(std::move(rust_entry.value));
  }

 private:
  ffi::Lister *lister_{nullptr};
};

Lister::Lister(ffi::Lister *lister)
    : lister_impl_(std::make_unique<ListerImpl>(lister)) {}

Lister::Lister(Lister &&other) noexcept = default;
Lister::~Lister() noexcept = default;

std::optional<Entry> Lister::Next() { return lister_impl_->next(); }

Lister::Iterator Lister::begin() { return Iterator(*this); }
Lister::Iterator Lister::end() { return Iterator(*this, true); }

// Lister::Iterator implementation
Lister::Iterator::Iterator(Lister &lister) : lister_(lister) {
  current_entry_ = lister_.Next();
}

Entry Lister::Iterator::operator*() { return current_entry_.value(); }

Lister::Iterator &Lister::Iterator::operator++() {
  if (current_entry_) {
    current_entry_ = lister_.Next();
  }
  return *this;
}

bool Lister::Iterator::operator!=(const Iterator &other) const {
  return current_entry_ != std::nullopt || other.current_entry_ != std::nullopt;
}

Lister::Iterator::Iterator(Lister &lister, bool /*end*/) noexcept
    : lister_(lister) {}

}  // namespace opendal
