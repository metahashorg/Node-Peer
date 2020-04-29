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

#include <gperftools/malloc_extension.h>
#include <signal.h>
#include <pthread.h>
#include <sniper/event/Loop.h>
#include <sniper/event/Sig.h>
#include <sniper/event/Timer.h>
#include <sniper/log/log.h>
#include <sniper/mh/Key.h>
#include <sniper/std/check.h>
#include <sniper/threads/Stop.h>
#include <sniper/event/Timer.h>

#include "Config.h"
#include "Proxy.h"
#include "stats/Stats.h"
#include "version.h"
#include "DomainGroup.h"


using namespace sniper;

void stop_signal(const event::loop_ptr& loop)
{
    log_warn("Stop signal. Exiting");
    threads::Stop::get().stop();
    loop->break_loop(ev::ALL);
}

static void sigsegv_handler(int sig)
{
    log_err("Error: signal {} ({})", strsignal(sig), sig);
    log_err("{}", stacktrace_to_string(boost::stacktrace::stacktrace()));
    exit(1);
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        log_info("Peer: {} ({}, {} UTC), rev {}", VERSION, __DATE__, __TIME__, GIT_SHA1);
        log_err("Run: peer_node config.conf network_file [key_file]");
        return EXIT_FAILURE;
    }

    auto loop = event::make_loop();
    if (!loop) {
        log_err("Main: cannot init event loop");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGSEGV, sigsegv_handler);
    signal(SIGABRT, sigsegv_handler);

    event::Sig sig_int(loop, SIGINT, stop_signal);
    event::Sig sig_iterm(loop, SIGTERM, stop_signal);


    try {
        log_info("Starting...");

        auto counters = make_shared<GlobalCounters>();
        sniper::Config config(argv[1], argv[2], loop);
        config.GetDomainGroup()->SetConfig(&config);
        config.GetDomainGroup()->StartWatchers();

        if (argc != 4) {
            log_err("Run: peer_node config.conf network_file key_file");
            return EXIT_FAILURE;
        }

        mh::Key key(argv[3]);
        log_info("mh_addr: {}", key.hex_addr());

        Proxy proxy(config, counters, key);
        Stats stats(config, counters, key);

        event::TimerRepeat timer_tcmalloc(loop, 10s, [] { MallocExtension::instance()->ReleaseFreeMemory(); });
        event::TimerRepeat timer_queue_size(loop, 100ms, [&counters, &proxy] { counters->queue = proxy.queue_size(); });

        log_info("Peer started");

        loop->run();
    }
    catch (std::exception& e) {
        log_err(e.what());
        return EXIT_FAILURE;
    }
    catch (...) {
        log_err("Non std::exception occured");
        return EXIT_FAILURE;
    }

    log_info("Peer stopped");
    return EXIT_SUCCESS;
}
