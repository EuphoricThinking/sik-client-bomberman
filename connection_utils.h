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

#include <unordered_set>
#include <set>

#include "constants.h"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;
using boost::asio::io_context;

using std::string;
using std::vector;

using std::exception;
using std::cerr;
using std::endl;
using std::cout;

using udp_buff_send = boost::array<uint8_t, max_udp_roundup>;
using input_mess = boost::array<uint8_t, max_input_mess_roundup>;
using tcp_buff_rec = boost::array<uint8_t, tcp_buff_default>;
using tcp_buff_send = std::vector<uint8_t>;

typedef struct Player {
    string name;
    string address;
} Player;

typedef struct Position {
    uint16_t x;
    uint16_t y;
} Position;

typedef struct Bomb {
    Position coordinates;
    uint16_t timer;
} Bomb;

typedef struct ServerMessageData {
    int8_t server_current_message_id = -1;

    // Hello
    bool is_hello_string_length_read = false;
    bool is_hello_string_read = false;

    // For both AcceptedPlayer and GameStarted
    bool is_player_header_read = false; // PlayerId and string length
    bool is_player_string_read = false;
    bool is_player_address_read = false;

    // Turn
    bool is_turn_header_read = false; // Turn number and list length

    size_t map_length = 0;
    size_t map_read_elements = 0;

    size_t list_length = 0;
    size_t list_read_elements = 0;

    size_t inner_event_list_length = 0;
    size_t inner_event_list_read_elements = 0;

    // Events
    int8_t event_id = -1;
    bool is_inner_event_header_read = false; // BombId, List PlayerId length
    bool is_robots_destroyed_read = false;
    bool is_blocks_destroyed_length_read = false;
} serverMessage;
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
    input_mess received_data_gui; // TODO will it work with the default constructor?
    tcp_buff_rec received_data_server;
    udp_buff_send data_to_send_gui;
    tcp_buff_send data_to_send_server;

    size_t num_bytes_to_read_server;

    ServerMessageData temp_process_server_mess;

    std::map<player_id_dt, Player> players;
    std::map<player_id_dt, Position> player_positions;
    //std::map<pos_x, std::unordered_set<pos_y>> blocks;
    std::set<std::pair<pos_x, pos_y>> blocks;
    std::map<std::pair<pos_x, pos_y>, timer_dt> bombs;

    std::set<std::pair<pos_x, pos_y>> explosions_temp;
    std::map<player_id_dt, score_dt> scores;
    std::unordered_set<player_id_dt> death_per_turn_temp;


    /*
     *  GUI -> client -> server
     */
    void process_data_from_gui(const boost::system::error_code& error,
                               std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) {

        }

        // Send to server
        boost::asio::async_write(socket_tcp_, boost::asio::buffer(data_to_send_server),
                                 boost::bind(&Client_bomberman::after_sent_data_to_server, this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void after_sent_data_to_server([[maybe_unused]] const boost::system::error_code& error,
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
        boost::asio::async_read(socket_tcp_,
                                boost::asio::buffer(received_data_server,
                                                    num_bytes_to_read_server),
                                    boost::bind(&Client_bomberman::after_receive_from_server,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
    }

    void after_receive_from_server(const boost::system::error_code& error,
                                   std::size_t read_bytes) {

        if (!error || error == boost::asio::error::eof || read_bytes > 0) { //read_bytes
            if (temp_process_server_mess.server_current_message_id == - 1) {
                // Check read one byte
                temp_process_server_mess.server_current_message_id =
                        (int8_t)received_data_server[0];
            }
//            if (false) { // If all the message bytes haven't been read; reading has to be continued
//                receive_from_gui_send_to_server();
//            } else {
//                process_data_from_server_send_to_gui();
//            }
        }
    }

    void process_data_from_server_send_to_gui() {

        socket_udp_.async_send(boost::asio::buffer(data_to_send_gui),
                               boost::bind(&Client_bomberman::after_send_to_gui,
                                           this,
                                           boost::asio::placeholders::error,
                                           boost::asio::placeholders::bytes_transferred));
    }

    void after_send_to_gui([[maybe_unused]] const boost::system::error_code& error,
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
        gameStarted(false),
        num_bytes_to_read_server(1),
        player_positions() //,
        // blocks(),
        // bombs()
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
