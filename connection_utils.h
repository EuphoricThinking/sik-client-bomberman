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
#include <utility>

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

using udp_buff_send = char[max_udp_roundup];
using input_mess = boost::array<uint8_t, max_input_mess_roundup>;
using tcp_buff_rec = uint8_t[tcp_buff_default];
using tcp_buff_send = char[tcp_buff_default];

void validate_data_compare(uint64_t should_not_be_smaller, uint64_t should_not_be_greater,
                           const string& mess) {
    if (should_not_be_smaller < should_not_be_greater) {
        cerr << mess << endl;

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
    uint8_t put_now;
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

class Client_bomberman {
private:
    tcp::socket socket_tcp_;
    tcp::resolver tcp_resolver_;

    udp::resolver udp_resolver_;
    udp::socket socket_udp_;
    udp::socket gui_socket_to_receive_;

    bool gameStarted;
    input_mess received_data_gui;
    tcp_buff_rec received_data_server;
    udp_buff_send data_to_send_gui;
    tcp_buff_send data_to_send_server;

    GameData game_status;

    std::map<player_id_dt, Player> players;
    std::map<player_id_dt, Position> player_positions;
    std::set<std::pair<pos_x, pos_y>> blocks;
    std::map<bomb_id_dt, Bomb> bombs;

    std::set<std::pair<pos_x, pos_y>> explosions_temp;
    std::map<player_id_dt, score_dt> scores;
    std::unordered_set<player_id_dt> death_per_turn_temp;
    std::set<std::pair<pos_x, pos_y>> blocks_exploded_temp;

    string server_name;
    string player_name;
    uint8_t message_id;

    const int list_blocks = 0;
    const int list_bombs = 1;
    const int list_explosions = 2;

    const int map_players = 0;
    const int map_positions = 1;
    const int map_score = 2;

    void update_player_scores() {
        for (auto dead_player : death_per_turn_temp) {
            auto player_score = scores.find(dead_player);
            player_score->second++;
        }
    }

    void update_bomb_timers() {
        for (auto iter = bombs.begin(); iter != bombs.end(); iter++) {
            Bomb& bomb_data = iter->second;
            if (bomb_data.put_now) {
                bomb_data.put_now = false;
            }
            else {
                bomb_data.timer--;
            }
        }
    }

    bool in_range(int to_test, int upper_limit) {
        if (to_test >= 0 && to_test < upper_limit) {
            return true;
        }
        else {
            return false;
        }
    }

    bool check_if_possible_to_propagate(int potential_x, int potential_y) {
        if (in_range(potential_x, game_status.size_x)
            && in_range(potential_y, game_status.size_y)
            && blocks_exploded_temp.find(make_pair(potential_x, potential_y))
            == blocks_exploded_temp.end()) {
                explosions_temp.insert(make_pair(potential_x,
                                             potential_y));

                return true;
        }
        else {
            return false;
        }
    }

    void add_explosion_cross(bomb_id_dt exploding_bomb_id) {
        auto exploded = bombs.find(exploding_bomb_id);

        if (exploded != bombs.end()) {
            position_dt centre_x = exploded->second.coordinates.x;
            position_dt centre_y = exploded->second.coordinates.y;

            for (int potential_x = centre_x;
                potential_x <= game_status.explosion_radius + centre_x;
                potential_x++) {
                    if (!check_if_possible_to_propagate(potential_x, centre_y)) {
                        break;
                    }
            }

            for (int potential_x = centre_x;
                 potential_x >= centre_x - game_status.explosion_radius;
                 potential_x--) {
                    if (!check_if_possible_to_propagate(potential_x, centre_y)) {
                        break;
                    }
            }

            for (int potential_y = centre_y;
                 potential_y <= game_status.explosion_radius + centre_y;
                 potential_y++) {
                if (!check_if_possible_to_propagate(centre_x, potential_y)) {
                    break;
                }
            }

            for (int potential_y = centre_y;
                 potential_y >= centre_y - game_status.explosion_radius;
                 potential_y--) {
                if (!check_if_possible_to_propagate(centre_x, potential_y)) {
                    break;
                }
            }
        }
    }

    /*
     *  GUI -> client -> server
     */
    bool is_valid_gui_message(size_t read_bytes) {
        if (read_bytes > max_input_message_bytes || read_bytes == 0) {
            return false;
        }

        uint8_t mess_id = received_data_gui[0];

        if (mess_id > max_gui_message_id) {
            return false;
        }
        else {
            if (mess_id == InputMessage::PlaceBlockGUI
                || mess_id == InputMessage::PlaceBombGUI) {
                    if (read_bytes != 1) {
                        return false;
                    }
            }
            else {
                if (read_bytes != 2) {
                    return false;
                }
            }
        }

        return true;
    }

    void process_data_from_gui(const boost::system::error_code& error,
                               std::size_t read_bytes) {
        if ((!error || error == boost::asio::error::eof) && is_valid_gui_message(read_bytes)) {
            size_t bytes_to_send = 0;

            if (!gameStarted) {
                data_to_send_server[0] = ClientMessage::Join;
                bytes_to_send++;

                data_to_send_server[bytes_to_send] = (char)player_name.length();
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
        else {
            receive_from_gui_send_to_server();
        }
    }

    void after_sent_data_to_server([[maybe_unused]] const boost::system::error_code& error,
                                   std::size_t) {
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
                                                    message_event_id_bytes),
                                    boost::bind(&Client_bomberman::check_message_id,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
    }

    void check_message_id(const boost::system::error_code& error,
                                   std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, 1, "No message id read\n");
            uint8_t message_type = received_data_server[0];

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
                                    1, false)); // how many times - single player

                    break;

                case (ServerMessage::GameStarted):
                    gameStarted = true;

                    boost::asio::async_read(socket_tcp_,
                        boost::asio::buffer(received_data_server,
                                            map_list_length),
                        boost::bind(&Client_bomberman::read_player_map_length,
                                    this,
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

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

                default:
                    cerr << "Incorrect message id\n";

                    exit(1);
            }
        }
    }

    /*
     *  Hello
     */
    void read_server_name_length (const boost::system::error_code& error,
                                  std::size_t read_bytes) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, 1, "Hello: incorrect server name length\n");

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
        if (!error || error == boost::asio::error::eof) {
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
                                        std::size_t read_bytes, int num_repetitions,
                                        bool is_game_started) {
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
                                num_repetitions, player_id, string_length_to_read,
                                is_game_started));
            }
            else {
                process_data_from_server_send_to_gui();
            }
        }
    }

    void read_player_name_string_and_address_length(const boost::system::error_code& error,
                                        std::size_t read_bytes, int num_repetitions,
                                        player_id_dt player_id, size_t name_length,
                                        bool is_game_started) {
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
                            player_name_temp, is_game_started));
        }
    }

    void read_player_address_string_add_player(const boost::system::error_code& error,
                                                    std::size_t read_bytes, int num_repetitions,
                                                    player_id_dt player_id, size_t address_length,
                                                    const string& player_name_temp,
                                                    bool is_game_started) {
        if (!error || error == boost::asio::error::eof || read_bytes > 0) {
            validate_data_compare(read_bytes, address_length, "Error player address");

            std::string player_address = "";

            if (address_length > 0) {
                player_address =
                        std::string(received_data_server,
                                    received_data_server + read_bytes);
            }

            players.insert(make_pair(player_id, Player{player_name_temp, player_address}));
            scores.insert(make_pair(player_id, 0));// TODO added

            if (--num_repetitions > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(received_data_server,
                                        player_id_name_header_length),
                    boost::bind(&Client_bomberman::read_player_id_and_name_length,
                                this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                num_repetitions, is_game_started));
            }
            else {
                if (is_game_started) {
                    receive_from_server_send_to_gui();
                }
                else {
                    process_data_from_server_send_to_gui();
                }
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

            if (map_length > 0) {
                boost::asio::async_read(socket_tcp_,
                    boost::asio::buffer(
                            received_data_server,
                            player_id_name_header_length),
                    boost::bind(
                            &Client_bomberman::read_player_id_and_name_length,
                            this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred,
                            map_length, true));
            }
            else {
                receive_from_server_send_to_gui(); //TODO added
            }
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
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, 1,
                                  "Turn - events list: no event id read\n");
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

                default:
                    cerr << "Incorrect event id\n";

                    exit(1);
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
                                   Bomb{Position{x, y}, game_status.bomb_timer, true}));

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
                                num_repetitions, player_id_list_length,
                                temp_bomb_id));
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
                                    num_repetitions, temp_bomb_id));
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
                         num_repetitions, map_list_length_dt player_id_left_elements,
                         bomb_id_dt bomb_id) {
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
                            num_repetitions, player_id_left_elements, bomb_id));
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
                            num_repetitions, bomb_id));
            }
        }
    }

    /*
     *  Bomb exploded - blocks destroyed
     */
    void read_blocks_destroyed_length (const boost::system::error_code& error,
                                       std::size_t read_bytes, map_list_length_dt
                                       num_repetitions, bomb_id_dt bomb_id) {
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
                            num_repetitions, blocks_destroyed_length, bomb_id));
            }
            else {
                add_explosion_cross(bomb_id);
                bombs.erase(bomb_id);

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
                                          map_list_length_dt blocks_position_left_elements,
                                          bomb_id_dt bomb_id) {
        if (!error || error == boost::asio::error::eof) {
            validate_data_compare(read_bytes, position_bytes,
                                  "Bomb exploded: incorrect position read");

            position_dt x = big_to_native(*(position_dt*)
                    (received_data_server));
            position_dt y = big_to_native(*(position_dt*)
                    (received_data_server + single_position_bytes));

            blocks.erase(make_pair(x, y));
            explosions_temp.insert(make_pair(x, y));
            blocks_exploded_temp.insert(make_pair(x, y));

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
                                num_repetitions, blocks_position_left_elements,
                                bomb_id));
            }
            else {
                add_explosion_cross(bomb_id);
                bombs.erase(bomb_id);

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
            }
            else {
                player_positions.insert(make_pair(player_id,
                                                  Position{x, y}));
            }

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
                scores.insert(make_pair(player_id, score));
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

    void write_map_to_buffer(size_t& bytes_to_send, int map_type) {
        size_t size_to_send = players.size();
        if (map_type == map_positions) {
            size_to_send = player_positions.size();
        }
        else if (map_type == map_score) {
            size_to_send = scores.size();
        }

        *(uint32_t *) (data_to_send_gui +
                       bytes_to_send) = (native_to_big((uint32_t )size_to_send));
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
        if (list_type == list_explosions) {
            length_to_send = explosions_temp.size();
        }
        else if (list_type == list_bombs) {
            length_to_send = bombs.size();
        }

        *(uint32_t *) (data_to_send_gui +
                       bytes_to_send) = (native_to_big((uint32_t)length_to_send));
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
                Bomb bomb_data = bomb.second;

                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb_data.coordinates.x);
                bytes_to_send += single_position_bytes;

                *(position_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb_data.coordinates.y);
                bytes_to_send += single_position_bytes;

                *(timer_dt *) (data_to_send_gui + bytes_to_send) =
                        native_to_big(bomb_data.timer);
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
        if (is_Lobby) {
            data_to_send_gui[bytes_to_send] = DrawMessage::Lobby;
        }
        else {
            data_to_send_gui[bytes_to_send] = DrawMessage::Game;
        }
        bytes_to_send++;
        data_to_send_gui[bytes_to_send] = (char)game_status.server_name.length();
        bytes_to_send++;

        strcpy(data_to_send_gui + bytes_to_send, game_status.server_name.c_str());
        bytes_to_send += game_status.server_name.length();

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
                           [[maybe_unused]] std::size_t sent_bytes) {
        // Repeat the cycle

        if (message_id == ServerMessage::Turn) {
            death_per_turn_temp.clear();
            explosions_temp.clear();
            blocks_exploded_temp.clear();
        }
        else if (message_id == ServerMessage::GameEnded) {
            scores.clear();
            bombs.clear();
            blocks.clear();
            player_positions.clear();
            players.clear();

            gameStarted = false;
        }

        receive_from_server_send_to_gui();
    }

public:
    Client_bomberman(io_context& io, const string& server_name, const string& server_port, const string& gui_name,
                     const string& gui_port, uint16_t client_port, string player_name)
    :
        socket_tcp_(io),
        tcp_resolver_(io),
        udp_resolver_(io),
        socket_udp_(io),
        gui_socket_to_receive_(io, udp::endpoint(udp::v6(), client_port)),
        gameStarted(false),
        player_positions(),
        player_name(std::move(player_name)),
        message_id(def_no_message)
{
        try {
//            memset(received_data_server, 0, tcp_buff_default);
            boost::asio::ip::tcp::no_delay option(true);

            tcp::resolver::results_type server_endpoints_ =
                    tcp_resolver_.resolve(server_name, server_port);
            boost::asio::connect(socket_tcp_, server_endpoints_);
            socket_tcp_.set_option(option);

            udp::resolver::results_type gui_endpoints_to_send_ = udp_resolver_.resolve(gui_name, gui_port); 
            boost::asio::connect(socket_udp_, gui_endpoints_to_send_);

            receive_from_server_send_to_gui();
            receive_from_gui_send_to_server();
        }
        catch (exception& e)
        {
            cerr << e.what() << endl;
        }

};

};

#endif //CLIENT_BOMBERMAN_CONNECTION_UTILS_H
