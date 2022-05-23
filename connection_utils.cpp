//
// Created by heheszek on 23.05.22.
//
#include "connection_utils.h"

TCP_connection::pointer TCP_connection::create(boost::asio::io_context& io_context)
    {
        return pointer(new TCP_connection(io_context));
    }

    tcp::socket& TCP_connection::socket()
    {
        return socket_;
    }

    void start()
    {
//        message_ = make_daytime_string();

        boost::asio::async_write(socket_, boost::asio::buffer(message_),
                                 boost::bind(&TCP_connection::handle_write, shared_from_this()));
    }

private:
    TCP_connection(boost::asio::io_context& io_context)
            : socket_(io_context)
    {
    }

    void handle_write()
    {
    }

    tcp::socket socket_;
    std::string message_;
};
