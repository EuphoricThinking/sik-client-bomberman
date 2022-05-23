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
#include <iostream>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;
using boost::asio::io_context;

using std::string;
using std::cerr;

/*
 * Acceptor
 * https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/reference/basic_socket_acceptor/bind/overload1.html
 */
class Client_bomberman {
private:
    io_context& io_;

    tcp::socket socket_tcp_;
    tcp::endpoint server_endpoint_;
    tcp::acceptor acceptor_;
    tcp::resolver tcp_resolver_;

    udp::resolver udp_resolver_;
    udp::socket socket_udp_;
    udp::endpoint gui_endpoint_;

public:
    Client_bomberman(io_context& io, string server_name, string server_port, const string gui_name,
                     string gui_port)
    :   io_(io),
        socket_tcp_(io),
        acceptor_(io),
        tcp_resolver_(io),
        //acceptor_(io, tcp::endpoint(tcp::v6(), server_port)),
        udp_resolver_(io),
        socket_udp_(io)
{
        try {
            //gui_endpoint_ = *udp_resolver_.resolve(udp::resolver::query(gui_name)).begin();
            gui_endpoint_ = *udp_resolver_.resolve(udp::v6(), gui_name,gui_name).begin();
            socket_udp_.open(udp::v6());

            server_endpoint_ = *tcp_resolver_.resolve(tcp::v6(), server_name, server_port).begin();
            acceptor_.open(tcp::v6());
            acceptor_.bind(server_endpoint_);
            //udp::socket s(io, gui_endpoint_);
            //socket_udp_ = s;
        }
        catch (std::exception& e)
        {
            cerr << e.what() << std::endl;
        }

};

};

#endif //CLIENT_BOMBERMAN_CONNECTION_UTILS_H
