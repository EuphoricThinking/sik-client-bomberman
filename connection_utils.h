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

using std::exception;
using std::cerr;
using std::endl;
using std::cout;

/*
 * Acceptor
 * https://www.boost.org/doc/libs/1_78_0/doc/html/boost_asio/reference/basic_socket_acceptor/bind/overload1.html
 */
class Client_bomberman {
private:
    //io_context& io_;

    tcp::socket socket_tcp_;
    //tcp::endpoint server_endpoints_;
    // tcp::acceptor acceptor_;
    tcp::resolver tcp_resolver_;

    udp::resolver udp_resolver_;
    udp::socket socket_udp_;
//    udp::endpoint gui_endpoint_to_send_;
//    udp::endpoint gui_endpoint_to_receive_;
    udp::socket gui_socket_to_receive_;

    bool gameStarted;

    void receive_from_server_send_to_gui() {

    }

    void receive_from_gui_send_to_server() {
        // Receive from GUI
        size_t gui_received_length;
    }
public:
    Client_bomberman(io_context& io, const string& server_name, const string& server_port, const string& gui_name,
                     const string& gui_port, uint16_t client_port)
    :   //io_(io),
        socket_tcp_(io),
        // acceptor_(io),
        tcp_resolver_(io),
        //acceptor_(io, tcp::endpoint(tcp::v6(), server_port)),
        udp_resolver_(io),
        socket_udp_(io),
        gui_endpoint_to_receive_(io, udp::endpoint(udp::v6(), client_port)),
        gameStarted(false)
{
        try {
            //gui_endpoint_to_send_ = *udp_resolver_.resolve(udp::resolver::query(gui_name)).begin();
            boost::asio::ip::tcp::no_delay option(true);
            socket_tcp_.set_option(option);

            tcp::resolver::results_type server_endpoints_ =
                    tcp_resolver_.resolve(server_name, server_port);
            boost::asio::connect(socket_tcp_, server_endpoints_);

            udp::resolver::results_type gui_endpoint_to_send_ = udp_resolver_.resolve(gui_name, gui_port);
            //socket_udp_.bind(udp::endpoint(udp::v6(), client_port));
            //socket_udp_.open(udp::v6());
            boost::asio::connect(socket_udp_, gui_endpoint_to_send_);

            //udp::resolver res(io_context)
            //auto endpoints = res.resolve(host, port)
            //gui_output.open(udp::v6())
            //gui_output.connect(endpoints->endpoint())

            //gui_input(io_context, udp::endpoint(udp::v6(), port))

            receive_from_server_send_to_gui();
            receive_from_gui_send_to_server();
            //acceptor_.open(tcp::v6());
            //acceptor_.bind(server_endpoints_);
            //udp::socket s(io, gui_endpoint_to_send_);
            //socket_udp_ = s;
        }
        catch (exception& e)
        {
            cerr << e.what() << endl;
        }

};

};

#endif //CLIENT_BOMBERMAN_CONNECTION_UTILS_H
