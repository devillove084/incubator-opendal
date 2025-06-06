// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

use dict_derive::FromPyObject;
use opendal as ocore;
use pyo3::pyclass;
use std::collections::HashMap;

#[pyclass(module = "opendal")]
#[derive(FromPyObject, Default)]
pub struct WriteOptions {
    pub append: Option<bool>,
    pub chunk: Option<usize>,
    pub content_type: Option<String>,
    pub content_disposition: Option<String>,
    pub cache_control: Option<String>,
    pub user_metadata: Option<HashMap<String, String>>,
}

impl From<WriteOptions> for ocore::options::WriteOptions {
    fn from(opts: WriteOptions) -> Self {
        Self {
            append: opts.append.unwrap_or(false),
            chunk: opts.chunk,
            content_type: opts.content_type,
            content_disposition: opts.content_disposition,
            cache_control: opts.cache_control,
            user_metadata: opts.user_metadata,
            ..Default::default()
        }
    }
}
