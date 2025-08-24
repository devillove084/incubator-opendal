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

#include <chrono>
#include <cstdio>
#include <ctime>

#include "lib.rs.h"
#include "opendal.hpp"
#include "utils/ffi_converter.hpp"
#include "utils/rust_converter.hpp"

namespace opendal {

std::optional<std::string> parse_optional_string(ffi::OptionalString &&s) {
  if (s.has_value) {
    return std::string(std::move(s.value));
  } else {
    return std::nullopt;
  }
}

std::optional<bool> parse_optional_bool(ffi::OptionalBool &&b) {
  if (b.has_value) {
    return b.value;
  } else {
    return std::nullopt;
  }
}

Metadata parse_meta_data(ffi::Metadata &&meta) {
  Metadata metadata;

  // Basic information
  metadata.type = static_cast<EntryMode>(meta.mode);
  metadata.content_length = meta.content_length;

  // HTTP-style headers
  metadata.cache_control = parse_optional_string(std::move(meta.cache_control));
  metadata.content_disposition =
      parse_optional_string(std::move(meta.content_disposition));
  metadata.content_md5 = parse_optional_string(std::move(meta.content_md5));
  metadata.content_type = parse_optional_string(std::move(meta.content_type));
  metadata.content_encoding =
      parse_optional_string(std::move(meta.content_encoding));
  metadata.etag = parse_optional_string(std::move(meta.etag));

  // Versioning information
  metadata.version = parse_optional_string(std::move(meta.version));
  metadata.is_current = parse_optional_bool(std::move(meta.is_current));
  metadata.is_deleted = meta.is_deleted;

  // Parse last_modified timestamp
  auto last_modified_str = parse_optional_string(std::move(meta.last_modified));
  if (last_modified_str.has_value()) {
    // Parse ISO 8601 string to time_point using strptime to avoid locale lock
    std::tm tm = {};
    const char *str = last_modified_str.value().c_str();

    // Parse ISO 8601 format: YYYY-MM-DDTHH:MM:SS
    int year, month, day, hour, minute, second;
    if (sscanf(str, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute,
               &second) == 6) {
      tm.tm_year = year - 1900;  // years since 1900
      tm.tm_mon = month - 1;     // months since January (0-11)
      tm.tm_mday = day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_sec = second;
      tm.tm_isdst = -1;  // let mktime determine DST

      std::time_t time_t_value = std::mktime(&tm);
      if (time_t_value != -1) {
        metadata.last_modified =
            std::chrono::system_clock::from_time_t(time_t_value);
      }
    }
  }

  return metadata;
}

class Operator::OperatorImpl {
 public:
  OperatorImpl(std::string_view scheme,
               const std::unordered_map<std::string, std::string> &config) {
    auto rust_map = rust::Vec<ffi::HashMapValue>();
    rust_map.reserve(config.size());

    for (auto &[k, v] : config) {
      rust_map.push_back({utils::rust_string(k), utils::rust_string(v)});
    }

    operator_ = ffi::new_operator(utils::rust_str(scheme), rust_map);
  }

  ~OperatorImpl() {
    if (operator_) {
      ffi::delete_operator(operator_);
      operator_ = nullptr;
    }
  }

  bool available() const { return operator_ != nullptr; }

  std::string read(std::string_view path) {
    auto rust_vec = operator_->read(utils::rust_str(path));
    return {rust_vec.begin(), rust_vec.end()};
  }

  void write(std::string_view path, std::string_view data) {
    rust::Vec<uint8_t> vec;
    std::copy(data.begin(), data.end(), std::back_inserter(vec));
    operator_->write(utils::rust_str(path), vec);
  }

  bool exists(std::string_view path) {
    return operator_->exists(utils::rust_str(path));
  }

  void create_dir(std::string_view path) {
    operator_->create_dir(utils::rust_str(path));
  }

  void copy(std::string_view src, std::string_view dst) {
    operator_->copy(utils::rust_str(src), utils::rust_str(dst));
  }

  void rename(std::string_view src, std::string_view dst) {
    operator_->rename(utils::rust_str(src), utils::rust_str(dst));
  }

  void remove(std::string_view path) {
    operator_->remove(utils::rust_str(path));
  }

  Metadata stat(std::string_view path) {
    return parse_meta_data(operator_->stat(utils::rust_str(path)));
  }

  std::vector<Entry> list(std::string_view path) {
    auto rust_vec = operator_->list(utils::rust_str(path));

    std::vector<Entry> entries;
    entries.reserve(rust_vec.size());
    for (auto &&entry : rust_vec) {
      entries.emplace_back(utils::parse_entry(std::move(entry)));
    }

    return entries;
  }

  ffi::Lister *get_lister(std::string_view path) {
    return operator_->lister(utils::rust_str(path));
  }

  ffi::Reader *get_reader(std::string_view path) {
    return operator_->reader(utils::rust_str(path));
  }

  Capability info() {
    auto op_info = operator_->info();
    return Capability{
        .stat = op_info.stat,
        .stat_with_if_match = op_info.stat_with_if_match,
        .stat_with_if_none_match = op_info.stat_with_if_none_match,
        .read = op_info.read,
        .read_with_if_match = op_info.read_with_if_match,
        .read_with_if_none_match = op_info.read_with_if_none_match,
        .read_with_override_cache_control =
            op_info.read_with_override_cache_control,
        .read_with_override_content_disposition =
            op_info.read_with_override_content_disposition,
        .read_with_override_content_type =
            op_info.read_with_override_content_type,
        .write = op_info.write,
        .write_can_multi = op_info.write_can_multi,
        .write_can_empty = op_info.write_can_empty,
        .write_can_append = op_info.write_can_append,
        .write_with_content_type = op_info.write_with_content_type,
        .write_with_content_disposition =
            op_info.write_with_content_disposition,
        .write_with_cache_control = op_info.write_with_cache_control,
        .write_multi_max_size = op_info.write_multi_max_size,
        .write_multi_min_size = op_info.write_multi_min_size,
        .write_total_max_size = op_info.write_total_max_size,
        .create_dir = op_info.create_dir,
        .delete_feature = op_info.delete_feature,
        .copy = op_info.copy,
        .rename = op_info.rename,
        .list = op_info.list,
        .list_with_limit = op_info.list_with_limit,
        .list_with_start_after = op_info.list_with_start_after,
        .list_with_recursive = op_info.list_with_recursive,
        .presign = op_info.presign,
        .presign_read = op_info.presign_read,
        .presign_stat = op_info.presign_stat,
        .presign_write = op_info.presign_write,
        .shared = op_info.shared,
    };
  }

 private:
  ffi::Operator *operator_{nullptr};
};

Operator::Operator(std::string_view scheme,
                   const std::unordered_map<std::string, std::string> &config)
    : operator_impl_(std::make_unique<OperatorImpl>(scheme, config)) {}

Operator::Operator(Operator &&other) noexcept = default;
Operator &Operator::operator=(Operator &&other) noexcept = default;
Operator::~Operator() noexcept = default;

bool Operator::Available() const { return operator_impl_->available(); }

std::string Operator::Read(std::string_view path) {
  return operator_impl_->read(path);
}

void Operator::Write(std::string_view path, std::string_view data) {
  operator_impl_->write(path, data);
}

bool Operator::Exists(std::string_view path) {
  return operator_impl_->exists(path);
}

bool Operator::IsExist(std::string_view path) { return Exists(path); }

void Operator::CreateDir(std::string_view path) {
  operator_impl_->create_dir(path);
}

void Operator::Copy(std::string_view src, std::string_view dst) {
  operator_impl_->copy(src, dst);
}

void Operator::Rename(std::string_view src, std::string_view dst) {
  operator_impl_->rename(src, dst);
}

void Operator::Remove(std::string_view path) { operator_impl_->remove(path); }

Metadata Operator::Stat(std::string_view path) {
  return operator_impl_->stat(path);
}

std::vector<Entry> Operator::List(std::string_view path) {
  return operator_impl_->list(path);
}

Lister Operator::GetLister(std::string_view path) {
  return Lister(operator_impl_->get_lister(path));
}

Reader Operator::GetReader(std::string_view path) {
  return Reader(operator_impl_->get_reader(path));
}

Capability Operator::Info() { return operator_impl_->info(); }

}  // namespace opendal
