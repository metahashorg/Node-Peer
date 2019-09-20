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

#include <sniper/mh/Key.h>
#include <sniper/std/memory.h>
#include <thread>
#include "Config.h"
#include "stats/Counters.h"

namespace sniper {

class Stats
{
public:
    Stats(const Config& config, shared_ptr<GlobalCounters> counters, const mh::Key& key);
    ~Stats();

private:
    void worker_noexcept() noexcept;
    void worker();

    const Config& _config;
    shared_ptr<GlobalCounters> _counters;
    const mh::Key& _key;
    std::thread _worker;
};

} // namespace sniper
