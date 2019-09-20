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

#include <sniper/std/string.h>

namespace sniper {

enum class ErrorCode
{
    Ok = 0,
    GetInfo = 1,
    HttpMethodNotSupported = -32000, // http method not supported
    EmptyPostData = -32001, // empty post data
    JsonParseError = -32700, // json parse error
    UnsupportedVersion = -32002, // unsupported version
    UnsupportedMethod = -32601, // unsupported method
    UnsupportedParamsType = -32003, // unsupported params type
    NoRequiredParams = -32004, // no required params or bad type
    InvalidTransaction = -32005, // Invalid transaction
    VerificationFailed = -32006 //
};

constexpr string_view get_error_message(ErrorCode code)
{
    switch (code) {
        case ErrorCode::HttpMethodNotSupported:
            return "http method not supported";
        case ErrorCode::EmptyPostData:
            return "empty post data";
        case ErrorCode::JsonParseError:
            return "json parse error";
        case ErrorCode::UnsupportedVersion:
            return "unsupported version";
        case ErrorCode::UnsupportedMethod:
            return "unsupported method";
        case ErrorCode::UnsupportedParamsType:
            return "unsupported params type";
        case ErrorCode::NoRequiredParams:
            return "no required params or bad type";
        case ErrorCode::InvalidTransaction:
            return "Invalid transaction";
        case ErrorCode::VerificationFailed:
            return "Verification failed";
        default:
            return "Invalid transaction";
    }

    return ""; // for gcc
}

} // namespace sniper
