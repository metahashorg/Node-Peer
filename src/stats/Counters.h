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

#include <cstdint>
#include <sniper/std/atomic.h>

namespace sniper {

template<typename T>
struct Counters final
{
    T total = 0;
    T trash = 0;
    T no_req = 0;
    T inv = 0;
    T inv_sign = 0;
    T success = 0;
    T queue = 0;
};

using LocalCounters = Counters<int64_t>;
using GlobalCounters = Counters<atomic<int64_t>>;

void merge_counters(const LocalCounters& local, GlobalCounters& global);
void clear_counters(LocalCounters& local);

} // namespace sniper
