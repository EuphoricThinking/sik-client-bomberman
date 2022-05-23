//
// Created by heheszek on 23.05.22.
//

#ifndef CLIENT_BOMBERMAN_CONNECTION_UTILS_H
#define CLIENT_BOMBERMAN_CONNECTION_UTILS_H

#include <boost/array.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

class TCP_connection
        : public boost::enable_shared_from_this<TCP_connection>
{
public:
    typedef boost::shared_ptr<TCP_connection> pointer;

    static pointer create(boost::asio::io_context& io_context);

    tcp::socket& socket();

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

class Client_bomberman;

#endif //CLIENT_BOMBERMAN_CONNECTION_UTILS_H
