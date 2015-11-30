#ifndef RESOLVER_CACHE_H
#define RESOLVER_CACHE_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <mutex>

#include <boost/asio.hpp>
#include "boost/fiber/yield.hpp"

#include "log.h"

namespace petrel {

/// Cache for boosts asio resolver
class resolver_cache {
  public:
    set_log_tag_default_priority("resolver_cache");

    /// Cache entry
    template<typename entry_type>
    struct entry_t {
        std::chrono::system_clock::time_point expires;
        std::vector<entry_type> entries;
    };

    /// Cache class
    template <typename proto_type>
    class cache : public std::unordered_map<std::string, entry_t<boost::asio::ip::basic_endpoint<proto_type>>> {
      public:
        using entry = entry_t<boost::asio::ip::basic_endpoint<proto_type>>;
        using resolver = boost::asio::ip::basic_resolver<proto_type>;
        using resolver_iterator = typename resolver::iterator;
        using query = typename resolver::query;
    };

    using tcp = boost::asio::ip::tcp;
    using udp = boost::asio::ip::udp;

    using tcp_cache = cache<tcp>;
    using udp_cache = cache<udp>;

    resolver_cache();
    
    /// This method behaves like the basic_resolver version. It checks the cache first, does a DNS lookup on a miss and
    /// updates the cache afterwards.
    template <typename proto_type>
    const typename cache<proto_type>::resolver_iterator async_resolve(boost::asio::io_service& iosvc,
                                                                      const std::string& host,
                                                                      const std::string& service,
                                                                      boost::fibers::asio::yield_t<> yield) {
        using iterator = typename cache<proto_type>::resolver_iterator;
        using query = typename cache<proto_type>::query;
        using resolver = typename cache<proto_type>::resolver;
        using entry = typename cache<proto_type>::entry;
        
        auto& cache = get_cache<proto_type>();
        auto& mtx = get_cache_mtx<proto_type>();
        std::string key = host + ":" + service;
        auto now = std::chrono::system_clock::now();

        // check the cache first
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto iter = cache.find(key);
            if (iter != cache.end() && iter->second.expires > now) {
                return iterator::create(iter->second.entries.begin(), iter->second.entries.end(), host, service);
            }
        }

        // cache miss or entry expired, do a dns lookup now
        log_debug("cache miss for " << host << ":" << service);
        query q(host, service);
        resolver r(iosvc);
        auto iter = r.async_resolve(q, yield);
        auto ret = iter;
        entry e;
        e.expires = now + std::chrono::minutes(5);
        for (; iter != iterator(); iter++) {
            e.entries.push_back(*iter);
        }
        std::lock_guard<std::mutex> lock(mtx);
        cache[key] = std::move(e);
        return ret;
    }

  private:
    int m_ttl;
    tcp_cache m_tcp_cache;
    std::mutex m_tcp_cache_mtx;
    udp_cache m_udp_cache;
    std::mutex m_udp_cache_mtx;

    template <typename proto_type>
    cache<proto_type>& get_cache();
    template <typename proto_type>
    std::mutex& get_cache_mtx();
};

}  // petrel

#endif  // RESOLVER_CACHE_H
