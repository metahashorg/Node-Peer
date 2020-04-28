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

#include "JsonTransaction.h"
#include "error_code.h"

namespace sniper {

namespace {

void remove_hex_prefix(string_view& str) noexcept
{
    if (str.size() > 2 && str[0] == '0' && str[1] == 'x')
        str.remove_prefix(2);
}

} // namespace

string_view JsonTransaction::to() const noexcept
{
    return _to;
}

string_view JsonTransaction::value() const noexcept
{
    return _value;
}

string_view JsonTransaction::fee() const noexcept
{
    return _fee;
}

string_view JsonTransaction::nonce() const noexcept
{
    return _nonce;
}

string_view JsonTransaction::data() const noexcept
{
    return _data;
}

string_view JsonTransaction::pubkey() const noexcept
{
    return _pubkey;
}

string_view JsonTransaction::sign() const noexcept
{
    return _sign;
}

ErrorCode JsonTransaction::parse(const rapidjson::Document& doc)
{
    if (!is_obj(doc, "params"))
        return ErrorCode::UnsupportedParamsType;

    const auto& params = doc["params"];

    _to = get_string_or_empty(params, "to");
    _value = get_string_or_empty(params, "value");
    _fee = get_string_or_empty(params, "fee");
    _nonce = get_string_or_empty(params, "nonce");
    _data = get_string_or_empty(params, "data");
    _pubkey = get_string_or_empty(params, "pubkey");
    _sign = get_string_or_empty(params, "sign");

    remove_hex_prefix(_to);
    remove_hex_prefix(_data);
    remove_hex_prefix(_pubkey);
    remove_hex_prefix(_sign);

    if (_to.empty() || _value.empty() || _nonce.empty() || _pubkey.empty() || _sign.empty())
        return ErrorCode::NoRequiredParams;

    return ErrorCode::Ok;
}

} // namespace sniper
