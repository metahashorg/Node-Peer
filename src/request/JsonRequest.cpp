// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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

#include "JsonRequest.h"
#include "error_code.h"
#include "version.h"

namespace sniper {

optional<variant<int64_t, string_view>> JsonRequest::id() const noexcept
{
    return _id;
}

string_view JsonRequest::version() const noexcept
{
    return _version;
}

string_view JsonRequest::method() const noexcept
{
    return _method;
}

bool JsonRequest::is_transaction() const noexcept
{
    return _method == "mhc_send" || _method == "mhc_test_send";
}

bool JsonRequest::is_test_ransaction() const noexcept
{
    return _method == "mhc_test_send";
}

ErrorCode JsonRequest::parse(string_view data, rapidjson::Document& doc)
{
    if (data.empty())
        return ErrorCode::EmptyPostData;

    if (doc.Parse(data.data(), data.size()).HasParseError())
        return ErrorCode::JsonParseError;

    if (is_string(doc, "id"))
        _id = get_string(doc, "id");
    else if (is_num(doc, "id"))
        _id = get_int64(doc, "id");

    if (is_string(doc, "jsonrpc"))
        _version = get_string(doc, "jsonrpc");

    if (is_string(doc, "method"))
        _method = get_string(doc, "method");

    if constexpr (PROXY_STRICT_JSONAPI) {
        if (_version != "2.0")
            return ErrorCode::UnsupportedVersion;
    }

    if (!is_transaction()) {
        if (_method == "getinfo")
            return ErrorCode::GetInfo;

        return ErrorCode::UnsupportedMethod;
    }

    return ErrorCode::Ok;
}

} // namespace sniper
