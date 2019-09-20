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

#include <rapidjson/document.h>
#include <sniper/crypto/secp.h>
#include <sniper/event/Loop.h>
#include <sniper/event/Timer.h>
#include <sniper/http/Client.h>
#include <sniper/http/Server.h>
#include <sniper/log/log.h>
#include <sniper/mh/TX.h>
#include <sniper/std/check.h>
#include <sniper/strings/hex.h>
#include <sniper/threads/Stop.h>
#include "Proxy.h"
#include "error_code.h"
#include "message.h"
#include "request/JsonRequest.h"
#include "request/JsonTransaction.h"

namespace sniper {

namespace {

cache::StringCache::unique process_error_code(const mh::Key& key, const JsonRequest& json, ErrorCode ec,
                                              LocalCounters& counters)
{
    switch (ec) {
        case ErrorCode::EmptyPostData:
            counters.trash++;
            return make_error_json(ec, {});
        case ErrorCode::JsonParseError:
            counters.trash++;
            return make_error_json(ec, {});
        case ErrorCode::UnsupportedVersion:
            counters.trash++;
            return make_error_json(ec, json);
        case ErrorCode::GetInfo:
            counters.trash++;
            return make_getinfo_json(key.hex_addr(), json);
        case ErrorCode::UnsupportedMethod:
            counters.inv++;
            return make_error_json(ec, json);
        case ErrorCode::UnsupportedParamsType:
            counters.no_req++;
            return make_error_json(ec, json);
        case ErrorCode::NoRequiredParams:
            counters.no_req++;
            return make_error_json(ec, json);
        case ErrorCode::InvalidTransaction:
            counters.inv++;
            return make_error_json(ec, json);
        case ErrorCode::VerificationFailed:
            counters.inv_sign++;
            return make_error_json(ErrorCode::InvalidTransaction, json);
        default:
            counters.trash++;
            return make_error_json(ErrorCode::JsonParseError, {});
    }
}

bool sign(string_view privkey, string_view data, string& sig_hex)
{
    array<unsigned char, 200> sig_der{}; // 74
    size_t sig_der_len = sig_der.size();
    if (!crypto::secp256k1_sign(privkey, data, sig_der.data(), sig_der_len))
        return false;

    strings::bin2hex_append(sig_der.data(), sig_der_len, sig_hex);
    return true;
}

intrusive_ptr<http::client::Request> make_http_request(const Config& config, const mh::Key& key,
                                                       cache::StringCache::unique&& raw_tx)
{
    auto req = http::client::make_request();

    req->method = http::client::Method::Post;
    req->url.set_domain(config.network());

    // QS
    auto qs = cache::StringCache::get_unique(400);
    qs->append("pubk=");
    qs->append(key.hex_pubkey());
    qs->append("&sign=");

    if (!sign(key.raw_privkey(), *raw_tx, *qs))
        return nullptr;

    req->url.set_query(*qs);

    req->set_data(std::move(raw_tx));

    return req;
}

} // namespace

Proxy::Proxy(const Config& config, shared_ptr<GlobalCounters> counters, const mh::Key& key) :
    _config(config), _global_counters(std::move(counters)), _key(key)
{
    check(_global_counters, "[Proxy] cannot use null counters");

    for (unsigned i = 0; i < _config.threads_count(); i++) {
        _queues.emplace_back(make_unique<atomic<int64_t>>(0));
        _workers.emplace_back(&Proxy::worker_noexcept, this, i);
    }
}

Proxy::~Proxy()
{
    for (auto& w : _workers)
        if (w.joinable())
            w.join();
}

void Proxy::worker_noexcept(unsigned thread_num) noexcept
{
    try {
        worker(thread_num);
    }
    catch (std::exception& e) {
        log_err(e.what());
    }
    catch (...) {
        log_err("[Proxy] non std::exception occured");
    }
}

void Proxy::worker(unsigned thread_num)
{
    auto loop = event::make_loop();
    check(loop, "[Proxy] cannot init event loop");

    LocalCounters counters;

    http::Client http_client(loop, _config.http_client_config());
    http::Server http_server(loop, _config.http_server_config());
    check(http_server.bind(_config.ip(), _config.port()), "[Proxy] cannot bind to {}:{}", _config.ip(), _config.port());

    deque<intrusive_ptr<http::client::Request>> out_queue;

    http_server.set_cb_request([&, this](const auto& http_conn, const auto& http_req, const auto& http_resp) {
        http_resp->code = http::ResponseStatus::OK;

        do {
            counters.total++;
            if (_config.thread_queue_size()
                && (counters.queue + static_cast<int64_t>(out_queue.size())) > _config.thread_queue_size()) {
                if (_config.reqs_dump_err())
                    log_crit("Status: Err, Error: Too many requests, Req: {}", http_req->data());

                http_resp->set_connection_close();
                http_resp->code = http::ResponseStatus::TOO_MANY_REQUESTS;
                break;
            }

            rapidjson::Document doc;
            JsonRequest json;
            if (auto ec = json.parse(http_req->data(), doc); ec != ErrorCode::Ok) {
                if (_config.reqs_dump_err() && ec != ErrorCode::GetInfo)
                    log_crit("Status: Err, Error: {}, Path: {}, QS:{}, Post: {}", get_error_message(ec),
                             http_req->path(), http_req->qs(), http_req->data());
                else if (_config.reqs_dump_ok() && ec == ErrorCode::GetInfo)
                    log_warn("Status: Info, Req: {}", http_req->data());

                http_resp->set_data(process_error_code(_key, json, ec, counters));
                break;
            }

            JsonTransaction json_tx;
            if (auto ec = json_tx.parse(doc); ec != ErrorCode::Ok) {
                if (_config.reqs_dump_err())
                    log_crit("Status: Err, Error: {}, Req: {}", get_error_message(ec), http_req->data());

                http_resp->set_data(process_error_code(_key, json, ec, counters));
                break;
            }

            mh::TX tx;
            if (!tx.load_hex(json_tx.to(), json_tx.value(), json_tx.fee(), json_tx.nonce(), json_tx.data(),
                             json_tx.pubkey(), json_tx.sign())) {
                if (_config.reqs_dump_err())
                    log_crit("Status: Err, Error: {}, Req: {}", get_error_message(ErrorCode::InvalidTransaction),
                             http_req->data());

                http_resp->set_data(process_error_code(_key, json, ErrorCode::InvalidTransaction, counters));
                break;
            }

            if (!verify(tx)) {
                if (_config.reqs_dump_err())
                    log_crit("Status: Err, Error: {}, Req: {}", get_error_message(ErrorCode::VerificationFailed),
                             http_req->data());

                http_resp->set_data(process_error_code(_key, json, ErrorCode::VerificationFailed, counters));
                break;
            }

            // good !
            counters.success++;
            http_resp->set_data(make_ok_json(tx.raw(), json));

            if (_config.reqs_dump_ok())
                log_warn("Status: Ok, Req: {}", http_req->data());

            if (!json.is_test_ransaction()) {
                if (http_client.send(make_http_request(_config, _key, tx.move())))
                    counters.queue++;
                else
                    log_err("Client error: cannot create request to verif");
            }
        } while (false);

        http_resp->add_header_nocopy("Access-Control-Allow-Origin: *\r\n");
        http_conn->send(http_resp);
    });


    http_client.set_cb([this, &http_client, &counters, &out_queue](auto&& req, auto&& resp) {
        if (resp->code() != 200) {
            if (req->generation() < _config.http_post_retries()) {
                if (http_client.send(std::move(req)))
                    return;

                log_err("Client error: cannot create request to verif");
            }
            else {
                out_queue.emplace_back(std::move(req));
            }
        }

        counters.queue--;
    });

    event::TimerRepeat timer_stop(loop, 1s, [&loop] {
        if (threads::Stop::get().is_stopped())
            loop->break_loop(ev::ALL);
    });

    event::TimerRepeat timer_update_counters(loop, 100ms, [this, &counters, thread_num, &out_queue] {
        _queues[thread_num]->store(counters.queue + out_queue.size());

        if (counters.total) {
            merge_counters(counters, *_global_counters);
            clear_counters(counters);
        }
    });

    event::TimerRepeat timer_out_queue(loop, 100ms, [&http_client, &out_queue, &counters] {
        if (out_queue.empty())
            return;

        for (size_t i = 0; i < std::min(200ul, out_queue.size()); i++) {
            if (http_client.send(std::move(out_queue.front())))
                counters.queue++;
            else
                log_err("Client error: cannot create request to verif");

            out_queue.pop_front();
        }
    });

    loop->run();
}

int64_t Proxy::queue_size() const noexcept
{
    int64_t size = 0;
    for (const auto& n : _queues)
        size += n->load();

    return size;
}

} // namespace sniper
