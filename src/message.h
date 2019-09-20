/*
 * Copyright (c) 2018 - 2019, MetaHash, Oleg Romanenko (oleg@romanenko.ro)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <sniper/cache/ArrayCache.h>
#include <sniper/std/string.h>

namespace sniper {

struct JsonRequest;
enum class ErrorCode;

/*
 * orig
 * {"result":"ok","error":"empty post data","id":null}
 *
 * strict
 * {"jsonrpc":"2.0","error":{"code":1,"message":"empty post data"},"id":null}
 *
 */
[[nodiscard]] cache::StringCache::unique make_error_json(ErrorCode code, const JsonRequest& req);

/*
 * orig
 * {"result":"ok","params":"tx_hash_hex","id":1}
 *
 * strict
 * {"jsonrpc":"2.0","result":{"status":"ok","params":"tx_hash_hex"},"id":1}
 *
 */
[[nodiscard]] cache::StringCache::unique make_ok_json(string_view raw_tx, const JsonRequest& req);

/*
 * orig
 * {"result":{"version":"2.0.1","mh_addr":"bla bla"},"id":1}
 *
 * strict
 * {"jsonrpc":"2.0","result":{"version":"2.0.1","mh_addr":"bla bla"},"id":1}
 *
 */
[[nodiscard]] cache::StringCache::unique make_getinfo_json(string_view mh_addr, const JsonRequest& req);

} // namespace sniper
