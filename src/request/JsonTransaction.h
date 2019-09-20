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

#include <sniper/std/rapidjson.h>
#include <sniper/std/string.h>

namespace sniper {

struct JsonRequest;
enum class ErrorCode;

/*
{
    "to": "0x00310693853b38a469b3b72121fac3a84fff6ece83680beaf6",
    "value": "1",
    "fee": "0",
    "nonce": "1",
    "data": "",
    "pubkey": "3056301006072a8648ce3d020106052b8104000a03420004101e02c08cad82d9cf5add6534e392b5e2be199f1d13defdd4ba919e9a11ca3776fdfe51136557020439dd0f58967c2e60e353bf20f2999f817e4c51c1425132",
    "sign": "3045022100fcf755ccb57e2343faf2c77235b5bb880a4a92e17add5e35c22c45cc553576b802200a4d2b9969b62fae1ed56f9907daf89f9282b9e3c872dfc4a88191c97e88f0b9"
}
*/
class JsonTransaction final
{
public:
    [[nodiscard]] string_view to() const noexcept;
    [[nodiscard]] string_view value() const noexcept;
    [[nodiscard]] string_view fee() const noexcept;
    [[nodiscard]] string_view nonce() const noexcept;
    [[nodiscard]] string_view data() const noexcept;
    [[nodiscard]] string_view pubkey() const noexcept;
    [[nodiscard]] string_view sign() const noexcept;

    [[nodiscard]] ErrorCode parse(const rapidjson::Document& doc);

private:
    string_view _to;
    string_view _value;
    string_view _fee;
    string_view _nonce;
    string_view _data;
    string_view _pubkey;
    string_view _sign;
};

} // namespace sniper
