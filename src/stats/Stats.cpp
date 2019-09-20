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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sniper/cache/ArrayCache.h>
#include <sniper/event/Loop.h>
#include <sniper/event/Timer.h>
#include <sniper/http/Client.h>
#include <sniper/log/log.h>
#include <sniper/std/check.h>
#include <sniper/threads/Stop.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Stats.h"
#include "version.h"

namespace sniper {

namespace {

string getHostName()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return string(hostname);
}

string getMyIp()
{
    string MyIP = "0.0.0.0";
    const char* statistics_server = "172.104.236.166";
    int statistics_port = 5797;

    struct sockaddr_in serv
    {};

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    //Socket could not be created
    if (sock < 0) {
        perror("Socket error");
        return "";
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(statistics_server);
    serv.sin_port = htons(statistics_port);

    connect(sock, (const struct sockaddr*)&serv, sizeof(serv));

    struct sockaddr_in name
    {};
    socklen_t namelen = sizeof(name);
    getsockname(sock, (struct sockaddr*)&name, &namelen);
    close(sock);

    char buffer[100];
    const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

    if (p != nullptr)
        MyIP = std::string(buffer);

    return MyIP;
}

} // namespace

Stats::Stats(const Config& config, shared_ptr<GlobalCounters> counters, const mh::Key& key) :
    _config(config), _counters(std::move(counters)), _key(key)
{
    check(_counters, "Stats: cannot use null counters");

    _worker = std::thread(&Stats::worker_noexcept, this);
}

Stats::~Stats()
{
    if (_worker.joinable())
        _worker.join();
}

void Stats::worker_noexcept() noexcept
{
    try {
        worker();
    }
    catch (std::exception& e) {
        log_err(e.what());
    }
    catch (...) {
        log_err("Stats: non std::exception occured");
    }
}

void Stats::worker()
{
    auto loop = event::make_loop();
    check(loop, "Stats: cannot init event loop");

    http::Client http_client(loop, _config.http_client_config());

    string my_ip = _config.ip().empty() ? getMyIp() : _config.ip();
    string my_hostname = getHostName();
    string my_addr(_key.hex_addr());

    event::TimerRepeat timer_stop(loop, 1s, [&loop] {
        if (threads::Stop::get().is_stopped())
            loop->break_loop(ev::ALL);
    });

    event::TimerRepeat timer_stats(loop, _config.stats_send_interval(), [&, this] {
        rapidjson::StringBuffer json;
        rapidjson::Writer<rapidjson::StringBuffer> w(json);
        w.SetMaxDecimalPlaces(0);

        auto ts = time_point_cast<seconds>(system_clock::now()).time_since_epoch();

        w.StartObject();
        {
            w.Key2("jsonrpc").String("2.0");
            w.Key2("params").StartObject();
            {
                w.Key2("network").String(_config.network().name());
                w.Key2("group").String("proxy");
                w.Key2("server").String(my_hostname);
                w.Key2("timestamp_ms").Int64(ts.count());

                w.Key2("metrics").StartArray();
                {
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->total.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps_trash");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->trash.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps_no_req");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->no_req.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps_inv");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->inv.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps_inv_sign");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->inv_sign.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("qps_success");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->success.exchange(0));
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("queue");
                        w.Key2("type").String("sum");
                        w.Key2("value").Int64(_counters->queue);
                    }
                    w.EndObject();
                    w.StartObject();
                    {
                        w.Key2("metric").String("ip");
                        w.Key2("type").String("none");
                        w.Key2("value").String(my_ip);
                    }
                    w.EndObject();

                    if (!my_addr.empty()) {
                        w.StartObject();
                        {
                            w.Key2("metric").String("mh_addr");
                            w.Key2("type").String("none");
                            w.Key2("value").String(my_addr);
                        }
                        w.EndObject();
                    }

                    w.StartObject();
                    {
                        w.Key2("metric").String("version");
                        w.Key2("type").String("none");
                        w.Key2("value").String(VERSION);
                    }
                    w.EndObject();
                }
                w.EndArray();
            }
            w.EndObject();
            w.Key2("id").Int(1);
        }
        w.EndObject();

        string_view str(json.GetString(), json.GetSize());

        if (_config.stats_dump_stdout())
            log_info("Stats:\n{}", str);

        if (_config.stats_send() && !http_client.post(_config.stats(), str))
            log_err("[Stats] cannot send");
    });

    loop->run();
}

} // namespace sniper
