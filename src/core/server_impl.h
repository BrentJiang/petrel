/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/program_options.hpp>
#include <nghttp2/asio_http2_server.h>

#include "lua_engine.h"
#include "resolver_cache.h"
#include "router.h"
#include "worker.h"
#include "metrics/registry.h"
#include "metrics/meter.h"
#include "metrics/timer.h"
#include "boost/http/buffered_socket.hpp"

namespace petrel {

namespace http = boost::http;
namespace http2 = nghttp2::asio_http2;
namespace ba = boost::asio;

/// The server class
class server_impl: boost::noncopyable {
  public:
    set_log_tag_default_priority("server");

    /// Ctor.
    server_impl(server* srv);

    /// Run the server and block.
    void run(bool async = false);

    /// Join the server if it was started in async mode.
    void join();

    /// Stop the server.
    void stop();

    /// Load lua scripts, call bootstrap etc
    void init();

    /// Install a lua function as handler for a path.
    void set_route(const std::string& path, const std::string& func);

    /// Return an io_service via round robin
    inline worker& get_worker() {
        auto next = m_next_worker.fetch_add(1, std::memory_order_relaxed);
        return *m_workers[next % m_workers.size()];
    }

    /// Return the lua engine
    inline lua_engine& get_lua_engine() {
        return m_lua_engine;
    }

    /// Return the resolver cache
    inline resolver_cache& get_resolver_cache() {
        return m_resolver_cache;
    }

    /// Return the router
    inline router& get_router() {
        return m_router;
    }

    /// Return the metrics registry
    inline metrics::registry& get_metrics_registry() {
        return m_registry;
    }

  private:
    /// Pointer to the server wrapper object
    server* m_server;

    int m_num_workers;
    std::vector<std::unique_ptr<worker>> m_workers;
    std::atomic<std::size_t> m_next_worker{0};

    std::function<void()> m_join_func;
    std::function<void()> m_stop_func;

    lua_engine m_lua_engine;
    resolver_cache m_resolver_cache;
    router m_router;
    metrics::registry m_registry;

    /// HTTP2 mode
    std::unique_ptr<http2::server::http2> m_http2_server;

    std::size_t m_num_routes = 0;

    metrics::meter::pointer m_metric_requests;
    metrics::meter::pointer m_metric_errors;
    metrics::timer::pointer m_metric_times;

    void run_http2_server(bool async = false);
    void run_http2_tls_server(bool async = false);
};

} // petrel

#endif  // SERVER_IMPL_H
