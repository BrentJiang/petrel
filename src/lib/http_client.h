#ifndef LIB_HTTP_CLIENT_H
#define LIB_HTTP_CLIENT_H

#include <boost/asio.hpp>

#include "library.h"
//#include "lua_engine.h"

namespace petrel {
namespace lib {

namespace ba = boost::asio;

class http_client : public library {
  public:
    http_client(lib_context* ctx) : library(ctx), m_sock(m_iosvc) {}
    static void init(lua_State*) {}
    int connect(lua_State* L);
    int disconnect(lua_State* L);
    int get(lua_State* L);

  private:
    ba::ip::tcp::socket m_sock;
    ba::streambuf m_req_buf;
    char m_res_buf[8192];
    std::string m_host;
    std::string m_port;
    bool m_connected;

    inline void close() {
        if (m_connected) {
            m_sock.close();
            m_connected = false;
        }
    }
};

}  // lib
}  // petrel

#endif  // LIB_HTTP_CLIENT_H
