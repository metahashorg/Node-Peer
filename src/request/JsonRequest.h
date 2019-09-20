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

#include <sniper/std/optional.h>
#include <sniper/std/rapidjson.h>
#include <sniper/std/string.h>
#include <sniper/std/variant.h>

namespace sniper {

enum class ErrorCode;

/*
{
    "id": 1,
    "jsonrpc": "2.0",
    "method": "mhc_send",
    ...
}
*/
class JsonRequest final
{
public:
    [[nodiscard]] optional<variant<int64_t, string_view>> id() const noexcept;
    [[nodiscard]] string_view version() const noexcept;
    [[nodiscard]] string_view method() const noexcept;
    [[nodiscard]] bool is_transaction() const noexcept;
    [[nodiscard]] bool is_test_ransaction() const noexcept;

    [[nodiscard]] ErrorCode parse(string_view data, rapidjson::Document& doc);

private:
    optional<variant<int64_t, string_view>> _id;
    string_view _version;
    string_view _method;
};

} // namespace sniper
