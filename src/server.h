#ifndef SERVER_H
#define SERVER_H

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
class server : boost::noncopyable {
  public:
    set_log_tag_default_priority("server");

    /// Ctor.
    server();

    /// Run the server and block.
    void run();

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
    int m_num_workers;
    std::vector<std::unique_ptr<worker>> m_workers;
    std::atomic<std::size_t> m_next_worker{0};

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

    void run_http2_server();
    void run_http2_tls_server();
};

} // petrel
#endif  // SERVER_H
