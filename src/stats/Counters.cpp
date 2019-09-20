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

#include "Counters.h"

namespace sniper {

void merge_counters(const LocalCounters& local, GlobalCounters& global)
{
    global.total += local.total;
    global.trash += local.trash;
    global.no_req += local.no_req;
    global.inv += local.inv;
    global.inv_sign += local.inv_sign;
    global.success += local.success;
}

void clear_counters(LocalCounters& local)
{
    local.total = 0;
    local.trash = 0;
    local.no_req = 0;
    local.inv = 0;
    local.inv_sign = 0;
    local.success = 0;
}

} // namespace sniper
