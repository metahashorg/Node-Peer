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

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sniper/crypto/hash.h>
#include <sniper/strings/hex.h>
#include "message.h"
#include "Config.h"
#include "error_code.h"
#include "request/JsonRequest.h"
#include "version.h"

namespace sniper {

cache::StringCache::unique make_error_json(ErrorCode code, const JsonRequest& req)
{
    rapidjson::StringBuffer json;
    rapidjson::Writer<rapidjson::StringBuffer> w(json);
    w.SetMaxDecimalPlaces(0);

    w.StartObject();
    {
        if constexpr (PROXY_STRICT_JSONAPI) {
            w.Key2("jsonrpc").String("2.0");
            w.Key2("error").StartObject();
            {
                w.Key2("code").Int(static_cast<int>(code));
                w.Key2("message").String(get_error_message(code));
            }
            w.EndObject();
        }
        else {
            w.Key2("result").String("ok");
            w.Key2("error").String(get_error_message(code));
        }

        if (req.id()) {
            if (std::holds_alternative<int64_t>(*req.id()))
                w.Key2("id").Int64(std::get<int64_t>(*(req.id())));
            else
                w.Key2("id").String(std::get<string_view>(*req.id()));
        }
        else if constexpr (PROXY_STRICT_JSONAPI) {
            w.Key2("id").Null();
        }
    }
    w.EndObject();

    auto str = cache::StringCache::get_unique(json.GetSize());
    str->assign(json.GetString(), json.GetSize());
    return str;
}

cache::StringCache::unique make_ok_json(string_view raw_tx, const JsonRequest& req)
{
    array<char, SHA256_DIGEST_LENGTH * 2> hash_hex{};
    {
        array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
        crypto::hash_sha256_2(raw_tx, hash);
        strings::bin2hex(hash, hash_hex);
    }

    rapidjson::StringBuffer json;
    rapidjson::Writer<rapidjson::StringBuffer> w(json);
    w.SetMaxDecimalPlaces(0);

    w.StartObject();
    {
        if constexpr (PROXY_STRICT_JSONAPI) {
            w.Key2("jsonrpc").String("2.0");
            w.Key2("result").StartObject();
            {
                w.Key2("status").String("ok");
                w.Key2("params").String(string_view(hash_hex.data(), hash_hex.size()));
            }
            w.EndObject();
        }
        else {
            w.Key2("result").String("ok");
            w.Key2("params").String(string_view(hash_hex.data(), hash_hex.size()));
        }

        if (req.id()) {
            if (std::holds_alternative<int64_t>(*req.id()))
                w.Key2("id").Int64(std::get<int64_t>(*(req.id())));
            else
                w.Key2("id").String(std::get<string_view>(*req.id()));
        }
        else if constexpr (PROXY_STRICT_JSONAPI) {
            w.Key2("id").Null();
        }
    }
    w.EndObject();

    auto str = cache::StringCache::get_unique(json.GetSize());
    str->assign(json.GetString(), json.GetSize());
    return str;
}

cache::StringCache::unique make_getinfo_json(string_view mh_addr, const JsonRequest& req)
{
    rapidjson::StringBuffer json;
    rapidjson::Writer<rapidjson::StringBuffer> w(json);
    w.SetMaxDecimalPlaces(0);

    w.StartObject();
    {
        if constexpr (PROXY_STRICT_JSONAPI)
            w.Key2("jsonrpc").String("2.0");

        w.Key2("result").StartObject();
        {
            w.Key2("version").String(VERSION);

            if (!mh_addr.empty())
                w.Key2("mh_addr").String(mh_addr);
        }
        w.EndObject();

        if constexpr (!PROXY_STRICT_JSONAPI)
            w.Key2("error").Null();

        if (req.id()) {
            if (std::holds_alternative<int64_t>(*req.id()))
                w.Key2("id").Int64(std::get<int64_t>(*(req.id())));
            else
                w.Key2("id").String(std::get<string_view>(*req.id()));
        }
        else if constexpr (PROXY_STRICT_JSONAPI) {
            w.Key2("id").Null();
        }
    }
    w.EndObject();

    auto str = cache::StringCache::get_unique(json.GetSize());
    str->assign(json.GetString(), json.GetSize());
    return str;
}

} // namespace sniper
