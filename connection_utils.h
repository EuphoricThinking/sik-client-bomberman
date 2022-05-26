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
#include <boost/endian/conversion.hpp>
#include <iostream>

#include <unordered_set>
#include <set>

#include "constants.h"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;
using boost::asio::io_context;
using boost::endian::big_to_native;
using boost::endian::native_to_big;

using std::string;
using std::vector;

using std::exception;
using std::cerr;
using std::endl;
using std::cout;
using std::make_pair;

using udp_buff_send = char[max_udp_roundup];//boost::array<uint8_t, max_udp_roundup>;
using input_mess = boost::array<uint8_t, max_input_mess_roundup>;
using tcp_buff_rec = uint8_t[tcp_buff_default]; //boost::array<uint8_t, tcp_buff_default>;
using tcp_buff_send = char[tcp_buff_default]; //std::vector<uint8_t>;

void validate_server_mess_id(uint8_t mess) {
    // negative values are cast to big positive values
    if (mess > max_server_mess_id) {
        cerr << "Incorrect message id\n";

        exit(1);
    }
}

void validate_data_compare(uint64_t should_not_be_smaller, uint64_t should_not_be_greater,
                           const string& mess) {
    if (should_not_be_smaller < should_not_be_greater) {
        cerr << mess << endl;

        exit(1);
    }
}
void validate_event_mess_id(uint8_t mess) {
    if (mess > max_event_mess_id) {
        cerr << "Incorrect event id\n";

        exit(1);
    }
}

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

typedef struct GameData {
    string server_name = "";
    uint8_t players_count = 0;
    uint16_t size_x = 0;
    uint16_t size_y = 0;

    uint16_t game_length = 0;
    uint16_t turn = 0;
    uint16_t explosion_radius = 0;
    uint16_t bomb_timer = 0;
} GameData;

typedef struct ServerMessageData {
    uint8_t server_current_message_id = def_no_message;

    // Hello
    bool is_hello_string_length_read = false;
    bool is_hello_string_read = false;

    uint8_t temp_player_id = 0;
    string temp_player_name;
    // For both AcceptedPlayer and GameStarted
    bool is_player_header_read = false; // PlayerId and string length
    bool is_player_string_read = false;
    bool is_player_address_length_read = false;
//    bool is_player_address_read = false;

    // Turn
    bool is_turn_header_read = false; // Turn number and list length

    bool game_started_map_read = false;
    size_t map_length = 0;
    size_t map_read_elements = 0;

    bool started_list_read = false;
    size_t list_length = 0;
    size_t list_read_elements = 0;

    bool started_inner_event_list_read = false;
    size_t inner_event_list_length = 0;
    size_t inner_event_list_read_elements = 0;

    // Events
    uint8_t event_id = def_no_message;
    bool is_inner_event_header_read = false; // BombId, List PlayerId length
    bool are_robots_destroyed_read = false;
//    bool is_blocks_destroyed_header_read = false;
    bool is_blocks_destroyed_length_read = false;

    bool is_game_ended_length_read = false;

//    bool is_player_moved_not_read = false;

//    bool is_bomb_placed_read = false;

    bool is_read_finished = false;
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
    GameData game_status;

    std::map<player_id_dt, Player> players;
    std::map<player_id_dt, Position> player_positions;
    //std::map<pos_x, std::unordered_set<pos_y>> blocks;
    std::set<std::pair<pos_x, pos_y>> blocks;
    std::map<bomb_id_dt, Bomb> bombs;

    std::set<std::pair<pos_x, pos_y>> explosions_temp;
    std::map<player_id_dt, score_dt> scores;
    std::unordered_set<player_id_dt> death_per_turn_temp;

    string server_name;
    //uint8_t send_to_gui_id;
    string player_name;
    uint8_t message_id;

    const int list_blocks = 0;
    const int list_bombs = 1;
    const int list_explosions = 2;

    const int map_players = 0;
    const int map_positions = 1;
    const int map_score = 2;

    Client_bomberman(const Client_bomberman&) = delete;

/*    void add_player_to_map_players(size_t read_bytes) {
        if (!temp_process_server_mess.is_player_header_read) {
            temp_process_server_mess.temp_player_id =
                    received_data_server[0];
            temp_process_server_mess.is_player_header_read = true;

            //move_further_in_buffer++;
            num_bytes_to_read_server =
                    received_data_server[1];

            receive_from_server_send_to_gui();
        }
        // Player name is saved in buffer
        else if (!temp_process_server_mess.is_player_string_read) {
            validate_data_compare(read_bytes, num_bytes_to_read_server,
                                  "Incorrect player name length");
            temp_process_server_mess.temp_player_name =
                    std::string(received_data_server, received_data_server + read_bytes);
            temp_process_server_mess.is_player_string_read = true;
            num_bytes_to_read_server = 1;

            receive_from_server_send_to_gui();
        }
        else if (!temp_process_server_mess.is_player_address_length_read) {
            num_bytes_to_read_server = received_data_server[0];
//            temp_process_server_mess.is_player_address_length_read = true;

            receive_from_server_send_to_gui();
        }
        else { //if (!temp_process_server_mess.is_player_address_read) {
            validate_data_compare(read_bytes, num_bytes_to_read_server,
                                  "Incorrect player name length");
            std::string player_address =
                    std::string(received_data_server, received_data_server + read_bytes);

            players.insert(make_pair(temp_process_server_mess.temp_player_id,
                                     Player{temp_process_server_mess.temp_player_name,
                                            player_address}));

            temp_process_server_mess.is_player_header_read = false;
            temp_process_server_mess.is_player_string_read = false;
            temp_process_server_mess.is_player_address_length_read = false;
            num_bytes_to_read_server = 1; // Next player id or message id
            // temp_process_server_mess.is_player_address_read = false;
        }
    }

    void read_and_process_bomb_exploded_from_server() {
        // TODO test whether it works
        for (auto iter = bombs.begin(); iter != bombs.end(); iter++) {
            Bomb& bomb_data = iter->second;
            bomb_data.timer--;
        }
    }
*/
    void update_player_scores() {
        for (auto dead_player : death_per_turn_temp) {
            auto player_score = scores.find(dead_player);
            player_score->second++;
        }
    }

    void update_bomb_timers() {
        for (auto iter = bombs.begin(); iter != bombs.end(); iter++) {
            Bomb& bomb_data = iter->second;
            bomb_data.timer--;
        }
    }
/*    void read_and_process_events(size_t read_bytes) {
        if (temp_process_server_mess.event_id == def_no_message) {
            temp_process_server_mess.event_id = received_data_server[0];

            validate_event_mess_id(received_data_server[0]);

            // Determine event id
            switch(received_data_server[0]) {
                case (Events::BombPlaced):
                    num_bytes_to_read_server = bomb_placed_header;

                    receive_from_server_send_to_gui();

                    break;

                case (Events::BombExploded):
                    num_bytes_to_read_server = bomb_id_list_length_header;

                    receive_from_server_send_to_gui();

                    break;

                case (Events::PlayerMoved):
                    num_bytes_to_read_server = player_id_pos_header;

                    receive_from_server_send_to_gui();

                    break;

                case (Events::BlockPlaced):
                    num_bytes_to_read_server = position_bytes;

                    receive_from_server_send_to_gui();

                    break;
            }
        }
        else {
            bomb_id_dt temp_bomb_id;
            position_dt x;
            position_dt y;
            std::map<bomb_id_dt, Bomb>::iterator iterBomb;
            player_id_dt player_id;
            std::map<player_id_dt, Position>::iterator iterPlayer;
            Position* temp_pos;

            switch(temp_process_server_mess.event_id) {
                case (Events::BombPlaced):
                    validate_data_compare(read_bytes, bomb_placed_header,
                                          "Incorrect bomb placed message");

                    temp_bomb_id = big_to_native(*(bomb_id_dt*)
                                            received_data_server);

                    x = big_to_native(*(position_dt*)
                            (received_data_server + bomb_id_bytes));
                    y = big_to_native(*(position_dt*)
                            (received_data_server + bomb_id_bytes + position_bytes));

                    bombs.insert(make_pair(temp_bomb_id,
                                           Bomb{Position{x, y}, game_status.bomb_timer}));

                    temp_process_server_mess.event_id = def_no_message;
                    num_bytes_to_read_server = 1; // event id or message id

                    if (++temp_process_server_mess.list_read_elements <
                            temp_process_server_mess.list_length) {
                        // next event, otherwise the list is read

                        receive_from_server_send_to_gui();
                    }

                    break;

                case (Events::BombExploded):
                    // Determine BombId, get length of the robots destroyed list
                    if (!temp_process_server_mess.is_inner_event_header_read) {
                        validate_data_compare(read_bytes, bomb_id_list_length_header,
                                 "Incorrect bomb exploded header");

                        temp_bomb_id = big_to_native(*(bomb_id_dt*)
                                received_data_server);

                        iterBomb = bombs.find(temp_bomb_id);
                        if (iterBomb != bombs.end()) {
                            x = iterBomb->second.coordinates.x;
                            y = iterBomb->second.coordinates.y;
                            explosions_temp.insert(make_pair(x, y));
                            bombs.erase(temp_bomb_id);

                            temp_process_server_mess.inner_event_list_length =
                                    big_to_native(*(map_list_length_dt *) (received_data_server
                                                                           + bomb_id_bytes));

                            temp_process_server_mess.is_inner_event_header_read = true;
                            num_bytes_to_read_server = player_id_bytes;

                            receive_from_server_send_to_gui();
                        }
                        else {
                            cerr << "Bomb not found\n";
                            exit(1);
                        }
                    }
                    else if (!temp_process_server_mess.are_robots_destroyed_read) {

                        // Probably this if is possible to delete?
                        // Reading dead players
                        if (temp_process_server_mess.inner_event_list_read_elements <
                            temp_process_server_mess.inner_event_list_length) {
                                death_per_turn_temp.insert(received_data_server[0]);


                                // Destroyed robots are read
                                if (++temp_process_server_mess.inner_event_list_read_elements
                                    >= temp_process_server_mess.inner_event_list_length) {
                                        num_bytes_to_read_server = map_list_length;
                                        temp_process_server_mess.are_robots_destroyed_read = true;

                                        temp_process_server_mess.inner_event_list_read_elements = 0;
                                }
                                receive_from_server_send_to_gui();
                        }
                    }
                    else if (!temp_process_server_mess.is_blocks_destroyed_length_read) {
                        // Reading blocks destroyed position length
                        temp_process_server_mess.inner_event_list_length =
                                big_to_native(*(map_list_length_dt*) received_data_server);
                        num_bytes_to_read_server = position_bytes;
                        temp_process_server_mess.is_blocks_destroyed_length_read = true;

                        receive_from_server_send_to_gui();
                    }
                    else {
                        // Reading destroyed blocks positions
                        if (temp_process_server_mess.inner_event_list_read_elements <
                            temp_process_server_mess.inner_event_list_length) {
                            x = big_to_native(*(position_dt*)
                                    (received_data_server));
                            y = big_to_native(*(position_dt*)
                                    (received_data_server + position_bytes));

                            blocks.erase(make_pair(x, y));
                            explosions_temp.insert(make_pair(x, y));

                            if (++temp_process_server_mess.inner_event_list_read_elements
                                >= temp_process_server_mess.inner_event_list_length) {
                                // Finished full event
                                temp_process_server_mess.inner_event_list_length = 0;
                                temp_process_server_mess.inner_event_list_read_elements = 0;
                                num_bytes_to_read_server = 1;

                                temp_process_server_mess.is_inner_event_header_read = false;
                                temp_process_server_mess.are_robots_destroyed_read = false;
                                temp_process_server_mess.is_blocks_destroyed_length_read = false;
                                temp_process_server_mess.event_id = def_no_message;

                                if (++temp_process_server_mess.list_read_elements
                                    != temp_process_server_mess.list_length) {
                                    // Isn't the last element

                                    receive_from_server_send_to_gui();
                                }
                            }
                        }
                    }

                    // receive_from_server_send_to_gui();

                    break;

                case (Events::PlayerMoved):
                    player_id = big_to_native(*(player_id_dt *) received_data_server);
                    x = big_to_native(*(position_dt*)
                            (received_data_server + player_id_bytes));
                    y = big_to_native(*(position_dt*)
                            (received_data_server + player_id_bytes + position_bytes));

                    iterPlayer = player_positions.find(player_id);
                    if (iterPlayer != player_positions.end()) {
                        temp_pos = &(iterPlayer->second);
                        temp_pos->x = x;
                        temp_pos->y = y;

                        temp_process_server_mess.event_id = def_no_message;
                        num_bytes_to_read_server = 1; // Next message or event id
                        if (++temp_process_server_mess.list_read_elements
                            != temp_process_server_mess.list_length) {
                            // Isn't the last element

                            receive_from_server_send_to_gui();
                        }
                    }

                    break;

                case (Events::BlockPlaced):
                    x = big_to_native(*(position_dt*)
                            (received_data_server));
                    y = big_to_native(*(position_dt*)
                            (received_data_server + position_bytes));
                    blocks.insert(make_pair(x, y));

                    temp_process_server_mess.event_id = def_no_message;
                    num_bytes_to_read_server = 1; // Message or event id
                    if (++temp_process_server_mess.list_read_elements
                        != temp_process_server_mess.list_length) {
                        // Isn't the last element

                        receive_from_server_send_to_gui();
                    }

                    break;
            }
        }
    }
    */


    /*
     *  GUI -> client -> server
     */
    void process_data_from_gui(const boost::system::error_code& error,
                               std::size_t read_bytes) {
        cout << "I have received" << endl;
        if (!error || error == boost::asio::error::eof) {
            cout << received_data_gui[0] << endl;
            size_t bytes_to_send = 0;

            if (read_bytes > max_input_message_bytes || read_bytes == 0) {
                cerr << "Incorrect GUI message length\n";

                exit(1);
            }

            if (!gameStarted) {
                data_to_send_server[0] = ClientMessage::Join;
                bytes_to_send++;

                strcpy(data_to_send_server + bytes_to_send, player_name.c_str());
                bytes_to_send += player_name.length();
            }
            else {
                uint8_t message_type = received_data_gui[0];

                switch (message_type) {
                    case (InputMessage::PlaceBombGUI):
                        validate_data_compare(read_bytes, 1, "Incorrect PlaceBomb gui message\n");

                        data_to_send_server[0] = ClientMessage::PlaceBomb;
                        bytes_to_send++;

                        break;

                    case (InputMessage::PlaceBlockGUI):
                        validate_data_compare(read_bytes, 1, "Incorrect PlaceBlock gui message\n");

                        data_to_send_server[0] = ClientMessage::PlaceBlock;
                        bytes_to_send++;

                        break;

                    case (InputMessage::MoveGUI):
                        data_to_send_server[0] = ClientMessage::Move;
                        bytes_to_send++;

                        data_to_send_server[bytes_to_send] = (char)received_data_gui[bytes_to_send];
                        bytes_to_send++;

                        break;

                }
            }


            // Send to server
            boost::asio::async_write(socket_tcp_,
                                     boost::asio::buffer(data_to_send_server, bytes_to_send),
                                     boost::bind(
                                             &Client_bomberman::after_sent_data_to_server,
                                             this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
        }
    }

    void after_sent_data_to_server([[maybe_unused]] const boost::system::error_code& error,
                                   std::size_t) {
        // Receive from GUI - repeat the cycle
        receive_from_gui_send_to_server();
    }

    void receive_from_gui_send_to_server() {
        // Receive from GUI
        cout << "I should receive\n";
        gui_socket_to_receive_.async_receive(
                boost::asio::buffer(received_data_gui),
                boost::bind(&Client_bomberman::process_data_from_gui, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));

    }






    /*
     *  Server -> client -> GUI  // TODO NEW
     */
    void receive_from_server_send_to_gui() {
        cout << "RECEIVE FROM SERVER" << endl;
        cout << "I'm here, bytes to read are: " << num_bytes_to_read_server << endl;
        boost::asio::async_read(socket_tcp_,
                                boost::asio::buffer(received_data_server,
                                                    message_event_id_bytes), //num_bytes_to_read_server),
                                    boost::bind(&Client_bomberman::check_message_id, //TODO 2 added // after_receive_from_server
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
    }

    void check_message_id(const boost::system::error_code& error,
                                   std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) { //} || read_bytes > 0) {
            uint8_t message_type = received_data_server[0];
            validate_server_mess_id(message_type);
            validate_data_compare(read_bytes, 1, "No message id read\n");

            message_id = message_type;

            switch (message_type) {
                case (ServerMessage::Hello):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            string_length_info),
                        boost::bind(&Client_bomberman::read_server_name_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                    break;

                case (ServerMessage::AcceptedPlayer):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            player_id_name_header_length),
                        boost::bind(&Client_bomberman::read_player_id_and_name_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred,
                                    1)); // how many times - single player

                    break;

                case (ServerMessage::GameStarted):
                    gameStarted = true;

                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            map_list_length),
                        boost::bind(&Client_bomberman::read_player_map_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred)); // how many times - single player

                    break;

                case (ServerMessage::Turn):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            turn_header),
                        boost::bind(&Client_bomberman::read_turn_and_list_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                    break;

                case (ServerMessage::GameEnded):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            map_list_length),
                        boost::bind(&Client_bomberman::read_score_map_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

                    break;
            }
        }
    }

    /*
     *  Hello
     */
    void read_server_name_length (const boost::system::error_code& error,
                                  std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) { //} || read_bytes > 0) {
            validate_data_compare(read_bytes, 1, "Hello: incorrect player name length\n");

            size_t read_length = received_data_server[0];

            boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(received_data_server,
                                        read_length),
                    boost::bind(&Client_bomberman::read_full_server_name_hello,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                read_length));
        }
    }

    void read_full_server_name_hello (const boost::system::error_code& error,
                                  std::size_t read_bytes, size_t name_length) {
        if (!error || error == boost::asio::error::eof) { //|| read_bytes > 0) {
            validate_data_compare(read_bytes, name_length,
                                  "Error in server name");

            game_status.server_name = "";
            if (read_bytes > 0) {
                game_status.server_name = std::string(received_data_server,
                                          received_data_server + read_bytes);
            }

            boost::asio::async_read(socket_tcp_,
                boost::asio::buffer(received_data_server,
                                    hello_body_length_without_string),
                boost::bind(&Client_bomberman::read_hello_body_without_string,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        }
    }

    void read_hello_body_without_string (const boost::system::error_code& error,
                                 std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, hello_body_length_without_string,
                                  "To little data in Hello\n");

            size_t move_further_in_buffer = 0;

            game_status.players_count = received_data_server[0];

            move_further_in_buffer++;
            game_status.size_x = big_to_native(*(uint16_t*)
                    (received_data_server + move_further_in_buffer));

            move_further_in_buffer += 2;
            game_status.size_y = big_to_native(*(uint16_t*)
                    (received_data_server + move_further_in_buffer));

            move_further_in_buffer += 2;
            game_status.game_length = big_to_native(*(uint16_t*)
                    (received_data_server + move_further_in_buffer));

            move_further_in_buffer += 2;
            game_status.explosion_radius = big_to_native(*(uint16_t*)
                    (received_data_server + move_further_in_buffer));

            move_further_in_buffer += 2;
            game_status.bomb_timer = big_to_native(*(uint16_t*)
                    (received_data_server + move_further_in_buffer));

            process_data_from_server_send_to_gui();
        }
    }

    /*
     * Accepted player
     */
    void read_player_id_and_name_length(const boost::system::error_code& error,
                                        std::size_t read_bytes, int num_repetitions) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            if (num_repetitions > 0) {
                validate_data_compare(read_bytes, player_id_name_header_length,
                                      "Error in reading player id and string name length");
                player_id_dt player_id = received_data_server[0];
                size_t string_length_to_read = received_data_server[1];

                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(received_data_server,
                                        string_length_to_read + 1),
                    boost::bind(&Client_bomberman::read_player_name_string_and_address_length,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions, player_id, string_length_to_read));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    void read_player_name_string_and_address_length(const boost::system::error_code& error,
                                        std::size_t read_bytes, int num_repetitions,
                                        player_id_dt player_id, size_t name_length) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, name_length, "Error player name");

            std::string player_name_temp = "";
            if (name_length > 0) {
                player_name_temp =
                        std::string(received_data_server,
                                    received_data_server + name_length);
            }

            size_t address_length = received_data_server[name_length];

            boost::asio::async_read(socket_tcp_,
                boost::asio::buffer(received_data_server,
                                    address_length),
                boost::bind(&Client_bomberman::read_player_address_string_add_player,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions, player_id, address_length,
                            player_name_temp));
        }
    }

    void read_player_address_string_add_player(const boost::system::error_code& error,
                                                    std::size_t read_bytes, int num_repetitions,
                                                    player_id_dt player_id, size_t address_length,
                                                    const string& player_name_temp) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, address_length, "Error player address");

            std::string player_address = "";

            if (address_length > 0) {
                player_address =
                        std::string(received_data_server,
                                    received_data_server + read_bytes);
            }

            players.insert(make_pair(player_id, Player{player_name_temp, player_address}));

            if (--num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(received_data_server,
                                        player_id_name_header_length),
                    boost::bind(&Client_bomberman::read_player_id_and_name_length,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    /*
     *  Game started
     */
    void read_player_map_length (const boost::system::error_code& error,
                                 std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, map_list_length, "GameStarted: incorrect map length");

            size_t map_length = big_to_native(*(map_list_length_dt *) received_data_server);

            boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(received_data_server,
                                        player_id_name_header_length),
                    boost::bind(&Client_bomberman::read_player_id_and_name_length,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                map_length));
        }
    }

    /*
     * Turn
     */
    void read_turn_and_list_length (const boost::system::error_code& error,
                                    std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, turn_header, "Incorrect turn header");

            game_status.turn =
                    big_to_native(*(uint16_t*) received_data_server);

            map_list_length_dt num_repetitions =
                    big_to_native(*(map_list_length_dt *)
                            (received_data_server + turn_bytes));

            if (num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                                        boost::asio::buffer(
                                                received_data_server,
                                                message_event_id_bytes),
                                        boost::bind(
                                                &Client_bomberman::read_event_id,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred,
                                                num_repetitions));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    /*
     *  Events
     */
    void read_event_id (const boost::system::error_code& error,
                                    std::size_t read_bytes, map_list_length_dt
                                    num_repetitions) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            uint8_t event_id = received_data_server[0];

            switch(event_id) {
                case (Events::BombPlaced):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                bomb_placed_header),
                        boost::bind(
                                &Client_bomberman::read_bomb_id_and_position,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));

                    break;

                case (Events::BombExploded):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                bomb_id_list_length_header),
                        boost::bind(
                                &Client_bomberman::read_bomb_id_and_player_id_length,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));

                    break;

                case (Events::PlayerMoved):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                player_id_pos_header),
                        boost::bind(
                                &Client_bomberman::read_player_id_and_position,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));

                    break;

                case (Events::BlockPlaced):
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                position_bytes),
                        boost::bind(
                                &Client_bomberman::read_block_position,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));

                    break;
            }
        }
    }

    void update_after_turn() {
        update_bomb_timers();
        update_player_scores();
    }

    /*
    * Bomb placed
    */
    void read_bomb_id_and_position (const boost::system::error_code& error,
                        std::size_t read_bytes, map_list_length_dt
                        num_repetitions) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, bomb_placed_header,
                                  "Error in reading bomb position");

            bomb_id_dt temp_bomb_id = big_to_native(*(bomb_id_dt*)
                    received_data_server);

            position_dt  x = big_to_native(*(position_dt*)
                    (received_data_server + bomb_id_bytes));
            position_dt y = big_to_native(*(position_dt*)
                    (received_data_server + bomb_id_bytes + single_position_bytes));

            bombs.insert(make_pair(temp_bomb_id,
                                   Bomb{Position{x, y}, game_status.bomb_timer}));

            if (--num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            message_event_id_bytes),
                    boost::bind(
                            &Client_bomberman::read_event_id,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions));
            }
            else {
                update_after_turn();

                process_data_from_server_send_to_gui();
            }
        }
    }

    /*
     *  Bomb exploded
     */
    void read_bomb_id_and_player_id_length (const boost::system::error_code& error,
                                            std::size_t read_bytes, map_list_length_dt
                                            num_repetitions) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, bomb_id_list_length_header,
                                  "Bomb exploded: incorrect length of bomb id"
                                  "or player id list length");

            bomb_id_dt temp_bomb_id = big_to_native(*(bomb_id_dt*)
                    received_data_server);

            auto iterBomb = bombs.find(temp_bomb_id);
            if (iterBomb != bombs.end()) {
                position_dt x = iterBomb->second.coordinates.x;
                position_dt y = iterBomb->second.coordinates.y;
                explosions_temp.insert(make_pair(x, y));
                bombs.erase(temp_bomb_id);

                map_list_length_dt player_id_list_length =
                        big_to_native(*(map_list_length_dt *) (received_data_server
                                                               + bomb_id_bytes));

                if (player_id_list_length > 0) {
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                player_id_bytes),
                        boost::bind(
                                &Client_bomberman::read_player_id,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions, player_id_list_length));
                }
                else {
                    boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            map_list_length),
                            boost::bind(
                                    &Client_bomberman::read_blocks_destroyed_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred,
                                    num_repetitions));
                }
            }
            else {
                cerr << "Bomb not found\n";
                exit(1);
            }
        }
    }

    /*
     *  Bomb exploded - player id list
     */
    void read_player_id (const boost::system::error_code& error,
                         std::size_t read_bytes, map_list_length_dt
                         num_repetitions, map_list_length_dt player_id_left_elements) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, player_id_bytes, "Error in player id");

            death_per_turn_temp.insert(received_data_server[0]);

            if (--player_id_left_elements > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            player_id_bytes),
                    boost::bind(
                            &Client_bomberman::read_player_id,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions, player_id_left_elements));
            }
            else {
                boost::asio::async_read(socket_tcp_,
                boost::asio::buffer(
                    received_data_server,
                    map_list_length),
                    boost::bind(
                            &Client_bomberman::read_blocks_destroyed_length,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions));
            }
        }
    }

    /*
     *  Bomb exploded - blocks destroyed
     */
    void read_blocks_destroyed_length (const boost::system::error_code& error,
                                       std::size_t read_bytes, map_list_length_dt
                                       num_repetitions) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, map_list_length,
                                  "Bomb exploded: Incorrect blocks destroyed length");

            map_list_length_dt blocks_destroyed_length =
                    big_to_native(*(map_list_length_dt*) received_data_server);

            if (blocks_destroyed_length > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            position_bytes),
                    boost::bind(
                            &Client_bomberman::read_blocks_destroyed_positions,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions, blocks_destroyed_length));
            }
            else {
                if (--num_repetitions > 0) {
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                message_event_id_bytes),
                        boost::bind(
                                &Client_bomberman::read_event_id,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));
                }
                else {
                    update_after_turn();

                    process_data_from_server_send_to_gui();
                }
            }
        }
    }

    void read_blocks_destroyed_positions (const boost::system::error_code& error,
                                          std::size_t read_bytes, map_list_length_dt
                                          num_repetitions,
                                          map_list_length_dt blocks_position_left_elements) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, position_bytes,
                                  "Bomb exploded: incorrect position read");

            position_dt x = big_to_native(*(position_dt*)
                    (received_data_server));
            position_dt y = big_to_native(*(position_dt*)
                    (received_data_server + single_position_bytes));

            blocks.erase(make_pair(x, y));
            explosions_temp.insert(make_pair(x, y));

            if (--blocks_position_left_elements > 0) {
                boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                position_bytes),
                        boost::bind(
                                &Client_bomberman::read_blocks_destroyed_positions,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions, blocks_position_left_elements));
            }
            else {
                if (--num_repetitions > 0) {
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                message_event_id_bytes),
                        boost::bind(
                                &Client_bomberman::read_event_id,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));
                }
                else {
                    update_after_turn();

                    process_data_from_server_send_to_gui();
                }
            }
        }
    }

    /*
     *  Player moved
     */
    void read_player_id_and_position (const boost::system::error_code& error,
                                      std::size_t read_bytes, map_list_length_dt
                                      num_repetitions) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, player_id_pos_header,
                                  "Turn: incorrect player position");

            player_id_dt player_id = big_to_native(*(player_id_dt *) received_data_server);
            position_dt x = big_to_native(*(position_dt*)
                    (received_data_server + player_id_bytes));
            position_dt y = big_to_native(*(position_dt*)
                    (received_data_server + player_id_bytes + single_position_bytes));

            auto iterPlayer = player_positions.find(player_id);
            if (iterPlayer != player_positions.end()) {
                Position & temp_pos = iterPlayer->second;
                temp_pos.x = x;
                temp_pos.y = y;

                if (--num_repetitions > 0) {
                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(
                                received_data_server,
                                message_event_id_bytes),
                        boost::bind(
                                &Client_bomberman::read_event_id,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions));
                }
                else {
                    update_after_turn();

                    process_data_from_server_send_to_gui();
                }
            }
            else {
                cerr << "Turn - Player Moved: player not found\n";

                exit(1);
            }
        }
    }

    /*
     *  Block placed
     */
    void read_block_position (const boost::system::error_code& error,
                              std::size_t read_bytes, map_list_length_dt
                              num_repetitions) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, position_bytes,
                                  "Turn - Block Placed: incorrect message length");

            position_dt x = big_to_native(*(position_dt*)
                    (received_data_server));
            position_dt y = big_to_native(*(position_dt*)
                    (received_data_server + single_position_bytes));
            blocks.insert(make_pair(x, y));

            if (--num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            message_event_id_bytes),
                    boost::bind(
                            &Client_bomberman::read_event_id,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions));
            }
            else {
                update_after_turn();

                process_data_from_server_send_to_gui();
            }
        }
    }

    /************* End of Turn *************/

    /*
     *  Game ended
     */
    void read_score_map_length (const boost::system::error_code& error,
                                std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, map_list_length,
                                  "GameEnded: incorrect message length");

            map_list_length_dt num_repetitions =
                    big_to_native(*(map_list_length_dt *)
                            received_data_server);

            if (num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            player_id_score_bytes),
                    boost::bind(
                            &Client_bomberman::read_player_id_score,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    void read_player_id_score (const boost::system::error_code& error,
                              std::size_t read_bytes, map_list_length_dt
                              num_repetitions) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, player_id_score_bytes,
                                  "Game ended: incorrect player id and score"
                                  "message length");

            player_id_dt player_id = received_data_server[0];

            score_dt score = big_to_native(*(score_dt *)
                    (received_data_server + player_id_bytes));

            auto iter = scores.find(player_id);
            if (iter != scores.end()) {
                iter->second = score;
            }
            else {
                cerr << "Game ended: player not found\n";

                exit(1);
            }

            if (--num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            player_id_score_bytes),
                    boost::bind(
                            &Client_bomberman::read_player_id_score,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            num_repetitions));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    /******* End of reading server messages ******/

  /*  void after_receive_from_server([[maybe_unused]] const boost::system::error_code& error,
                                   [[maybe_unused]] std::size_t read_bytes) {
        if (received_data_server[0] == ServerMessage::Hello) {
            cout << "HELLLLLO\n"; // id raead
            size_t strlength = string_length_info;
            boost::asio::async_read(socket_tcp_,
                                    boost::asio::buffer(received_data_server,
                                                        strlength),
                                    boost::bind(&Client_bomberman::after_receive_from_server_hel,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
    }

    void after_receive_from_server_hel([[maybe_unused]] const boost::system::error_code& error,
                                       [[maybe_unused]] std::size_t read_bytes) {
        size_t strlength = received_data_server[0];
        cout << "num bytes to read " << strlength << endl;

        boost::asio::async_read(socket_tcp_,
                                boost::asio::buffer(received_data_server,
                                                    strlength),
                                boost::bind(&Client_bomberman::after_receive_from_server_hel2,
                                            this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void after_receive_from_server_hel2([[maybe_unused]] const boost::system::error_code& error,
                                       std::size_t read_bytes) {
        cout << "i have read for a string: " << read_bytes << endl;
        server_name = std::string(received_data_server, received_data_server + read_bytes);
        cout << server_name << endl;

        boost::asio::async_read(socket_tcp_,
                                boost::asio::buffer(received_data_server,
                                                    hello_body_length_without_string),
                                boost::bind(&Client_bomberman::after_receive_from_server_hel3,
                                            this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
    void after_receive_from_server_hel3([[maybe_unused]] const boost::system::error_code& error,
                                        [[maybe_unused]] std::size_t read_bytes) {
        cout << server_name << endl;
    }

    void after_receive_from_server2(const boost::system::error_code& error,
                                   std::size_t read_bytes) {

        if (!error || error == boost::asio::error::eof || read_bytes > 0) { //read_bytes
            if (temp_process_server_mess.server_current_message_id == def_no_message) {
                // Check read one byte
                validate_server_mess_id(received_data_server[0]);
                cout << "read num " << received_data_server[0] << "read bytes " << read_bytes << endl;

                temp_process_server_mess.server_current_message_id =
                        received_data_server[0];

                cout << "read num " << temp_process_server_mess.server_current_message_id << "read bytes " << read_bytes << endl;

                switch (temp_process_server_mess.server_current_message_id) {
                    case (ServerMessage::AcceptedPlayer):
                        num_bytes_to_read_server = player_id_name_header_length;

                        break;

                    case (ServerMessage::Hello):
                        cout << "HELLO \n";
                        num_bytes_to_read_server = string_length_info;

                        break;

                    case (ServerMessage::GameStarted):
                        num_bytes_to_read_server = map_list_length;
                        gameStarted = true;

                        break;

                    case (ServerMessage::Turn):
                        num_bytes_to_read_server = turn_header;

                        break;

                    case (ServerMessage::GameEnded):
                        num_bytes_to_read_server = map_list_length;

                        break;
                }

                receive_from_server_send_to_gui();
            }
            else {
                int move_further_in_buffer = 0;

                switch (temp_process_server_mess.server_current_message_id) {
                    case (ServerMessage::Hello):
                        if (!temp_process_server_mess.is_hello_string_length_read) {
                            cout << "FIRST Save string hello length: " << (int)received_data_server[0] << endl;
                            num_bytes_to_read_server = received_data_server[0]; // String to read
                            temp_process_server_mess.is_hello_string_length_read = true;
//                             size_t kurwaaaa = num_bytes_to_read_server;
//
//                            boost::asio::async_read(socket_tcp_,
//                                                    boost::asio::buffer(received_data_server,
//                                                                        kurwaaaa),
//                                                    boost::bind(&Client_bomberman::after_receive_from_server,
//                                                                this,
//                                                                boost::asio::placeholders::error,
//                                                                boost::asio::placeholders::bytes_transferred));
//                            return;


                            receive_from_server_send_to_gui();
                        } // else removed?
                        if (!temp_process_server_mess.is_hello_string_read) {
                            cout << "SECOND Enter here" << endl;
                            cout << "SECOND Save string hello; saved: " << read_bytes << " intended: " << num_bytes_to_read_server << endl;
                            validate_data_compare(read_bytes, num_bytes_to_read_server,
                                            "Error in server name");

                            cout << "Save string hello; saved: " << read_bytes << " intended: " << num_bytes_to_read_server << endl;
                            server_name = std::string(received_data_server, received_data_server + read_bytes);
                            temp_process_server_mess.is_hello_string_read = true;
                            num_bytes_to_read_server = hello_body_length_without_string;
                            cout << "server name READ: " << server_name << endl;

                            receive_from_server_send_to_gui();
                        }
                        // Full body is read
                        cout << "Received from server: " << read_bytes << " to be read " << num_bytes_to_read_server << endl;
                        validate_data_compare(read_bytes, num_bytes_to_read_server,
                                              "To little data in Hello\n");
                        // game_status.players_count = (*(uint8_t*) received_data_server);
                        game_status.players_count = received_data_server[0];

                        move_further_in_buffer++;
                        game_status.size_x = big_to_native(*(uint16_t*)
                                (received_data_server + move_further_in_buffer));

                        move_further_in_buffer += 2;
                        game_status.size_y = big_to_native(*(uint16_t*)
                                (received_data_server + move_further_in_buffer));

                        move_further_in_buffer += 2;
                        game_status.game_length = big_to_native(*(uint16_t*)
                                (received_data_server + move_further_in_buffer));

                        move_further_in_buffer += 2;
                        game_status.explosion_radius = big_to_native(*(uint16_t*)
                                (received_data_server + move_further_in_buffer));

                        move_further_in_buffer += 2;
                        game_status.bomb_timer = big_to_native(*(uint16_t*)
                                (received_data_server + move_further_in_buffer));

                        move_further_in_buffer = 0;
                        temp_process_server_mess.is_hello_string_length_read = false;
                        temp_process_server_mess.is_hello_string_read = false;

                        break;

                    case (ServerMessage::AcceptedPlayer):
                        add_player_to_map_players(read_bytes);

                        break;

                    case (ServerMessage::GameStarted):
                        if (!temp_process_server_mess.game_started_map_read) {
                            temp_process_server_mess.map_length =
                                    received_data_server[0];
                            temp_process_server_mess.game_started_map_read = true;
                            num_bytes_to_read_server = player_id_name_header_length;

                            receive_from_server_send_to_gui();
                        }
                        else {
                            if (temp_process_server_mess.map_read_elements <
                                temp_process_server_mess.map_length) {
                                    add_player_to_map_players(read_bytes);

                                    if (++temp_process_server_mess.map_read_elements <
                                            temp_process_server_mess.map_length) {

                                        receive_from_server_send_to_gui();
                                    }
                                    else {
                                        temp_process_server_mess.game_started_map_read = false;
                                        temp_process_server_mess.map_length = 0;
                                        temp_process_server_mess.map_read_elements = 0;
                                    }
                            }
                        }

                        break;

                    case (ServerMessage::Turn):
                        // Turn number and events list length are not read
                        if (!temp_process_server_mess.is_turn_header_read) {
                            game_status.turn =
                                    big_to_native(*(uint16_t*) received_data_server);

                            //move_further_in_buffer += 2;
                            temp_process_server_mess.list_length =
                                    big_to_native(*(map_list_length_dt *)
                                            (received_data_server + turn_bytes)); //move_further_in_buffer));

                            temp_process_server_mess.is_turn_header_read = true;

                            // Read event id
                            num_bytes_to_read_server = 1;

                            receive_from_server_send_to_gui();
                        }
                        else {
                            if (temp_process_server_mess.list_read_elements <
                                    temp_process_server_mess.list_length) {
                                read_and_process_events(read_bytes);
                            }

                            // Returned from read_and_process_events() instead of
                            // calling receive_from_server_send_to_gui()
                            temp_process_server_mess.is_turn_header_read = false;
                            temp_process_server_mess.list_read_elements = 0;
                            temp_process_server_mess.list_length = 0;

                            update_bomb_timers();
                            update_player_scores();
                        }

                        break;

                    case (ServerMessage::GameEnded):
                        // Reading map length
                        if (!temp_process_server_mess.is_game_ended_length_read) {
                            temp_process_server_mess.map_length =
                                    big_to_native(*(map_list_length_dt *)
                                        received_data_server);
                            temp_process_server_mess.is_game_ended_length_read = true;
                            num_bytes_to_read_server = player_id_score_bytes;

                            receive_from_server_send_to_gui();
                        }

                        // Reading elements
                        if (temp_process_server_mess.map_read_elements <
                                temp_process_server_mess.map_length) {
                            player_id_dt player_id = received_data_server[0];

                            score_dt score = big_to_native(*(score_dt *)
                                    (received_data_server + 1));

                            auto iter = scores.find(player_id);
                            if (iter != scores.end()) {
                                iter->second = score;
                            }

                            if (++temp_process_server_mess.map_read_elements <
                                    temp_process_server_mess.map_length) {
                                receive_from_server_send_to_gui();
                            }
                            else {
                                temp_process_server_mess.is_game_ended_length_read = false;
                                temp_process_server_mess.map_length = 0;
                                temp_process_server_mess.map_read_elements = 0;
                            }

                        }

                        break;

                }
            }
//            if (false) { // If all the message bytes haven't been read; reading has to be continued
//                receive_from_gui_send_to_server();
//            } else {
//                process_data_from_server_send_to_gui();
//            }
            if (temp_process_server_mess.server_current_message_id !=
                ServerMessage::GameStarted) {
                process_data_from_server_send_to_gui();
            }
        }
    } */

    void write_map_to_buffer(size_t& bytes_to_send, int map_type) {
        size_t size_to_send = players.size();
        if (map_type == map_positions) size_to_send = player_positions.size();
        else if (map_type == map_score) size_to_send = scores.size();

        *(uint32_t *) (data_to_send_gui +
                       bytes_to_send) = (uint32_t)(native_to_big(size_to_send));
        bytes_to_send += map_list_length;

        if (map_type == map_players) {
            for (auto &player: players) {
                data_to_send_gui[bytes_to_send] = (char) player.first; // Player id
                bytes_to_send++;

                const Player &player_data = player.second;
                // Player name
                data_to_send_gui[bytes_to_send] = (char) player_data.name.length();
                bytes_to_send++;

                strcpy(data_to_send_gui + bytes_to_send,
                       player_data.name.c_str());
                bytes_to_send += player_data.name.length();

                // Player address
                data_to_send_gui[bytes_to_send] = (char) player_data.address.length();
                bytes_to_send++;

                strcpy(data_to_send_gui + bytes_to_send,
                       player_data.address.c_str());
                bytes_to_send += player_data.address.length();
            }
        }
        else if (map_type == map_positions) {
            for (auto & player_position : player_positions) {
                data_to_send_gui[bytes_to_send] = (char)player_position.first;
                bytes_to_send++;

                Position current_position = player_position.second;

                *(uint16_t *) (data_to_send_gui +
                               bytes_to_send) = (native_to_big(current_position.x));
                bytes_to_send += single_position_bytes;

                *(uint16_t *) (data_to_send_gui +
                               bytes_to_send) = (native_to_big(current_position.y));
                bytes_to_send += single_position_bytes;
            }
        }
        else if (map_type == map_score) {
            for (auto & player_score : scores) {
                data_to_send_gui[bytes_to_send] = (char)player_score.first;
                bytes_to_send++;

                *(score_dt *)(data_to_send_gui + bytes_to_send) = native_to_big(player_score.second);
                bytes_to_send += score_bytes;
            }
        }
    }

    void write_list_to_buffer(size_t& bytes_to_send, int list_type) {
        size_t length_to_send = blocks.size();
        if (list_type == list_explosions) length_to_send = explosions_temp.size();
        else if (list_type == list_bombs) length_to_send = bombs.size();

        *(uint32_t *) (data_to_send_gui +
                       bytes_to_send) = (uint32_t)(native_to_big(length_to_send));
        bytes_to_send += map_list_length;

        if (list_type == list_blocks) {
            for (const auto & block : blocks) {
                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(block.first);
                bytes_to_send += single_position_bytes;

                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(block.second);
                bytes_to_send += single_position_bytes;
            }
        }
        else if (list_type == list_bombs) {
            for (const auto & bomb : bombs) {
                *(bomb_id_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb.first);
                bytes_to_send += bomb_id_bytes;

                Position bomb_position = bomb.second.coordinates;
                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb_position.x);
                bytes_to_send += single_position_bytes;

                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb_position.y);
                bytes_to_send += single_position_bytes;

                *(timer_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb.second.timer);
                bytes_to_send += timer_bytes;
            }
        }
        else if (list_type == list_explosions) {
            for (const auto & explosion : explosions_temp) {
                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(explosion.first);
                bytes_to_send += single_position_bytes;

                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(explosion.second);
                bytes_to_send += single_position_bytes;
            }
        }
    }

    size_t create_udp_message(bool is_Lobby) {
        size_t bytes_to_send = 0;

        data_to_send_gui[0] = (char)server_name.length();
        bytes_to_send++;

        strcpy(data_to_send_gui + bytes_to_send, server_name.c_str());
        bytes_to_send += server_name.length();

        if (is_Lobby) {
            data_to_send_gui[bytes_to_send] = (char)game_status.players_count;
            bytes_to_send++;
        }

        *(uint16_t*)(data_to_send_gui + bytes_to_send) = (native_to_big(game_status.size_x));
        bytes_to_send += lobby_exc_pc_bytes;

        *(uint16_t*)(data_to_send_gui + bytes_to_send) = (native_to_big(game_status.size_y));
        bytes_to_send += lobby_exc_pc_bytes;

        *(uint16_t*)(data_to_send_gui + bytes_to_send) = (native_to_big(game_status.game_length));
        bytes_to_send += lobby_exc_pc_bytes;

        if (!is_Lobby) {
            *(uint16_t *) (data_to_send_gui + bytes_to_send) = (native_to_big(
                    game_status.turn));
            bytes_to_send += lobby_exc_pc_bytes;
        }
        else {
            *(uint16_t *) (data_to_send_gui + bytes_to_send) = (native_to_big(
                    game_status.explosion_radius));
            bytes_to_send += lobby_exc_pc_bytes;

            *(uint16_t *) (data_to_send_gui + bytes_to_send) = (native_to_big(
                    game_status.bomb_timer));
            bytes_to_send += lobby_exc_pc_bytes;
        }

        // players: Map<PlayerId, Player>
        write_map_to_buffer(bytes_to_send, map_players);

        if (!is_Lobby) {
            // Player position
            write_map_to_buffer(bytes_to_send, map_positions);
            write_list_to_buffer(bytes_to_send, list_blocks);
            write_list_to_buffer(bytes_to_send, list_bombs);
            write_list_to_buffer(bytes_to_send, list_explosions);
            write_map_to_buffer(bytes_to_send, map_score);
        }

        return bytes_to_send;
    }

    void process_data_from_server_send_to_gui() {
        //uint8_t message_type = temp_process_server_mess.server_current_message_id;
        cout << "I should send to gui\n";
        uint8_t message_type = message_id;

        if (message_type != ServerMessage::GameStarted) {
            size_t bytes_to_send = 0;

            if (message_type == ServerMessage::Hello ||
                message_type == ServerMessage::GameEnded
                || message_type == ServerMessage::AcceptedPlayer) {
                bytes_to_send = create_udp_message(true);
            } else {
                bytes_to_send = create_udp_message(false);
            }

            socket_udp_.async_send(
                    boost::asio::buffer(data_to_send_gui, bytes_to_send),
                    boost::bind(&Client_bomberman::after_send_to_gui,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
        }
    }

    void after_send_to_gui([[maybe_unused]] const boost::system::error_code& error,
                           std::size_t) {
        // Repeat the cycle
        //num_bytes_to_read_server = 1;
        //uint8_t message_type = temp_process_server_mess.server_current_message_id;
        //uint8_t message_type = message_id;
        cout << "I have sent to gui\n";

        if (message_id == ServerMessage::Turn) {
            death_per_turn_temp.clear();
            explosions_temp.clear();
        }
        else if (message_id == ServerMessage::GameEnded) {
            scores.clear();
            bombs.clear();
            blocks.clear();
            player_positions.clear();
            players.clear();

            gameStarted = false;
        }

        //temp_process_server_mess.server_current_message_id = def_no_message;

        receive_from_server_send_to_gui();
    }

public:
    Client_bomberman(io_context& io, const string& server_name, const string& server_port, const string& gui_name,
                     const string& gui_port, uint16_t client_port, string player_name)
    :   //io_(io),
        socket_tcp_(io),
        // acceptor_(io),
        tcp_resolver_(io),
        //acceptor_(io, tcp::endpoint(tcp::v6(), server_port)),
        udp_resolver_(io),
        socket_udp_(io),
        gui_socket_to_receive_(io, udp::endpoint(udp::v6(), client_port)),
        gameStarted(false),
        num_bytes_to_read_server(1), //TODO change
        player_positions(),
        // send_to_gui_id(0),
        player_name(player_name),
        message_id(def_no_message)//,+
        // blocks(),
        // bombs()
{
        try {
            cout << "buffer size: " << sizeof(received_data_server) << endl;
            memset(received_data_server, 0, tcp_buff_default);
            //gui_endpoint_to_send_ = *udp_resolver_.resolve(udp::resolver::query(gui_name)).begin();
            cout << "entered client" << endl;
            boost::asio::ip::tcp::no_delay option(true);
            cout << "Found option" << endl;
            //socket_tcp_.set_option(option);
            cout << "Set option " << server_name << " " << server_port << endl;

            tcp::resolver::results_type server_endpoints_ =
                    tcp_resolver_.resolve(tcp::v6(), server_name, server_port);
            cout << "tcp resolved" << endl;
            boost::asio::connect(socket_tcp_, server_endpoints_);
            socket_tcp_.set_option(option);
            cout << "tcp connected" << endl;
/*            auto endpoints = tcp_resolver_.resolve(server_name, server_port);
            socket_tcp_.open(tcp::v6());
            socket_tcp_.connect(endpoints->endpoint());
            socket_tcp_.set_option(option); */

 /*           udp::resolver::results_type gui_endpoints_to_send_ = udp_resolver_.resolve(udp::v6(), gui_name, gui_port);
            cout << "udp endpoints resolved\n";
            boost::asio::connect(socket_udp_, gui_endpoints_to_send_);
            cout << "udp connected" << endl;
 */

            //socket_udp_.bind(udp::endpoint(udp::v6(), client_port));
            //socket_udp_.open(udp::v6());

            //udp::resolver res(io_context)
            //auto endpoints = res.resolve(host, port)
            //gui_output.open(udp::v6())
            //gui_output.connect(endpoints->endpoint())
            auto endpoints = udp_resolver_.resolve(gui_name, gui_port);
            socket_udp_.open(udp::v6());
            socket_udp_.connect(endpoints->endpoint());

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
