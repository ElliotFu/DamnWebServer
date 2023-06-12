#include "http_server.h"

#include "http_common_define.h"
#include "http_context.h"
#include "http_response.h"
#include "net/tcp_connection.h"

void default_http_callback(const Http_Request&, Http_Response* resp)
{
    resp->set_status_code(Http_Response::NOT_FOUND);
    resp->set_status_message("Not Found");
    resp->set_close_connection(true);
}

Http_Server::Http_Server(Event_Loop* loop,
                         const sockaddr_in& listen_addr,
                         const string& name)
    : server_(loop, listen_addr, name),
      http_callback_(default_http_callback)
{
    server_.set_connection_callback(
        std::bind(&Http_Server::on_connection, this, std::placeholders::_1));
    server_.set_message_callback(
        std::bind(&Http_Server::on_message, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
    server_.set_thread_num(4);
}

void Http_Server::start()
{
    // LOG_WARN << "HttpServer[" << server_.name()
    //          << "] starts listening on " << server_.ipPort();
    server_.start();
}

void Http_Server::on_connection(const Tcp_Server::Tcp_Connection_Ptr& conn)
{
    if (conn->connected()) {
        // LOG_INFO << "New connection arrived.";
    } else {
        // LOG_INFO << "Connection closed.";
    }
}

void Http_Server::on_message(const Tcp_Server::Tcp_Connection_Ptr& conn,
                             Buffer* buf,
                             Timestamp receive_time)
{
    // LOG_INFo << "Http server::on message";
    std::unique_ptr<Http_Context> context(new Http_Context());
    if (!context->parse_request(buf, receive_time)) {
        // LOG_INFO << "Parse request failed.";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    if (context->got_all()) {
        // LOG_INFO << "Parse request success.";
        on_request(conn, context->request());
        context->reset();
    }
}

void Http_Server::on_request(const Tcp_Server::Tcp_Connection_Ptr& conn,
                             const Http_Request& req)
{
    const std::string& connection = req.get_header("Connection");
    bool close = connection == "close" ||
                 (req.get_version() == HTTP10 && connection != "Keep-Alive");
    Http_Response response(close);
    http_callback_(req, &response);
    Buffer buf;
    response.append_to_buffer(&buf);
    conn->send(&buf);
    if (response.close_connection()) {
        conn->shutdown();
    }
}