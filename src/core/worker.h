/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef WORKER_H
#define WORKER_H

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/fiber/all.hpp>

#include "log.h"
#include "session.h"

namespace petrel {

namespace ba = boost::asio;
namespace bs = boost::system;
namespace bf = boost::fibers;

class server;

/// Each io_service object runs in its own worker thread.
class worker : boost::noncopyable {
  public:
    set_log_tag_default_priority("worker");

    /// Ctor.
    worker() : m_work(m_iosvc) {}

    /// Dtor.
    ~worker();

    /// Create an acceptor for a tcp endpoint.
    void add_endpoint(ba::ip::tcp::endpoint& ep, bs::error_code& ec);

    /// Add a session to this worker
    void add_session(session::pointer new_session);

    /// Start a worker thread and return.
    void start(server& srv);

    /// Stop a worker
    void stop();

    /// Entry point.
    void run(server& srv);

    /// Block until the thread is finished.
    void join();

    /// Return the io_service object of the worker.
    inline ba::io_service& io_service() { return m_iosvc; }

  private:
    ba::io_service m_iosvc;
    ba::io_service::work m_work;
    std::thread m_thread;
    std::atomic_bool m_stop{false};

    std::vector<session::pointer> m_new_sessions;
    bf::mutex m_new_session_mtx;
    bf::condition_variable m_new_session_cv;

    std::vector<ba::ip::tcp::acceptor> m_acceptors;

    static void do_accept(ba::ip::tcp::acceptor& acceptor, server& srv);
};

}  // petrel

#endif  // WORKER_H
