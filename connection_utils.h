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
using std::vector;

using std::exception;
using std::cerr;
using std::endl;
using std::cout;

using data = vector<uint8_t>;

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
    data received_data_gui;
    data received_data_server;
    data data_to_send_gui;
    data data_to_send_server;

    /*
     *  GUI -> client -> server
     */
    void process_data_from_gui(const boost::system::error_code& error,
                               std::size_t) {
        if (!error) {

        }

        // Send to server
        boost::asio::async_write(socket_tcp_, boost::asio::buffer(data_to_send_server),
                                 boost::bind(&Client_bomberman::after_sent_data_to_server, this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void after_sent_data_to_server(const boost::system::error_code& error,
                                   std::size_t) {
        // Receive from GUI - repeat the cycle
        receive_from_gui_send_to_server();
    }

    void receive_from_gui_send_to_server() {
        // Receive from GUI
        gui_socket_to_receive_.async_receive(
                boost::asio::buffer(received_data_gui),
                boost::bind(&Client_bomberman::process_data_from_gui, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));

    }

    /*
     *  Server -> client -> GUI
     */
    void receive_from_server_send_to_gui() {
        socket_tcp_.async_read_some(boost::asio::buffer(received_data_server),
                                    boost::bind(&Client_bomberman::after_receive_from_server,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
    }

    void after_receive_from_server(const boost::system::error_code& error,
                                   std::size_t) {

        if (false) { // If all the message bytes haven't been read; reading has to be continued
            receive_from_gui_send_to_server();
        }
        else {
            process_data_from_server_send_to_gui();
        }
    }

    void process_data_from_server_send_to_gui() {

        socket_udp_.async_send(boost::asio::buffer(data_to_send_gui),
                               boost::bind(&Client_bomberman::after_send_to_gui,
                                           this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
    }

    void after_send_to_gui(const boost::system::error_code& error,
                           std::size_t) {
        // Repeat the cycle
        receive_from_server_send_to_gui();
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
        gui_socket_to_receive_(io, udp::endpoint(udp::v6(), client_port)),
        gameStarted(false)
{
        try {
            //gui_endpoint_to_send_ = *udp_resolver_.resolve(udp::resolver::query(gui_name)).begin();
            boost::asio::ip::tcp::no_delay option(true);
            socket_tcp_.set_option(option);

            tcp::resolver::results_type server_endpoints_ =
                    tcp_resolver_.resolve(server_name, server_port);
            boost::asio::connect(socket_tcp_, server_endpoints_);

            udp::resolver::results_type gui_endpoints_to_send_ = udp_resolver_.resolve(gui_name, gui_port);
            //socket_udp_.bind(udp::endpoint(udp::v6(), client_port));
            //socket_udp_.open(udp::v6());
            boost::asio::connect(socket_udp_, gui_endpoints_to_send_);

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
