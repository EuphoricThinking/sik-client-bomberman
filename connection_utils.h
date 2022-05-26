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

typedef struct Player {
    string name;
    string address;
} Player;

typedef struct Position {
    uint16_t x;
    uint16_t y;
} Position;

/*
 *  The field `placed_now` expresses that the bomb has been placed in the current
 *  turn. The author didn't choose to use timer+1 because of the risk of overflow.
 */
typedef struct Bomb {
    Position coordinates;
    uint16_t timer;
    uint8_t placed_now;
} Bomb;

/*
 * Information about the current game
 */
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

/*
 * The presented solution uses two separate flows of connection
 *      - server -> client -> gui
 *      - gui -> client -> server
 *
 * Subsequent operations related only to one of the presented flows
 * are executed in asynchronous functions and their callbacks,
 * all performed in a class Client_bomberman
 */
class Client_bomberman {
private:
    tcp::socket socket_tcp_;
    tcp::resolver tcp_resolver_;

    udp::resolver udp_resolver_;
    udp::socket socket_udp_;
    udp::socket gui_socket_to_receive_;

    std::atomic<bool> gameStarted;
    input_mess received_data_gui;
    tcp_buff_rec received_data_server;
    udp_buff_send data_to_send_gui;
    tcp_buff_send data_to_send_server;

    GameData game_status;

    std::map<player_id_dt, Player> players;
    std::map<player_id_dt, Position> player_positions;
    std::set<std::pair<pos_x, pos_y>> blocks;
    std::map<bomb_id_dt, Bomb> bombs;
    std::map<player_id_dt, score_dt> scores;

    /*
     * Used for results evaluation during each turn
     *      - explosions_temp: temporary storage for explosion positions
     *      - death_per_turn_temp: unique player ids, enable determining
     *      the score of each player regardless of the number of events
     *      that may interfere with each other but with the same result
     *      - blocks_exploded_temp: stores positions of the blocks
     *      which have been destroyed by the explosion but simultaneously
     *      have stopped its propagation; provided by server
     */
    std::set<std::pair<pos_x, pos_y>> explosions_temp;
    std::unordered_set<player_id_dt> death_per_turn_temp;
    std::set<std::pair<pos_x, pos_y>> blocks_exploded_temp;

    string server_name;
    string player_name; // client name
    uint8_t message_id;

    /*
     * Constants used for merged functions responsible for writing data
     * into the buffer to be sent
    */
    const int list_blocks = 0; // Left for cohesion
    const int list_bombs = 1;
    const int list_explosions = 2;

    const int map_players = 0;
    const int map_positions = 1;
    const int map_score = 2;

    void update_player_scores();

    void update_bomb_timers();

    /*
     * Checks if the explosion can propagate over the specified field:
     *      - whether the coordinates are bound within the possible range
     *      determined by the size of the board
     *      - whether the explosion has not encountered the block which
     *      has been marked as destroyed by the server and added to explosion
     *      points in a different function
     */
    bool check_if_possible_to_propagate(int potential_x, int potential_y);

    /*
     * Determines the coordinates of the exploded fields
     */
    void add_explosion_cross(bomb_id_dt exploding_bomb_id);


    /*
     *  GUI -> client -> server
     */
    bool is_valid_gui_message(size_t read_bytes);

    // Reads the message and creates the message to send
    void process_data_from_gui(const boost::system::error_code& error,
                               std::size_t read_bytes);

    void after_sent_data_to_server(
                        [[maybe_unused]] const boost::system::error_code& error,
                        std::size_t);

    void receive_from_gui_send_to_server();






    /*
     *  Server -> client -> GUI
     */
    void receive_from_server_send_to_gui();

    void check_message_id(const boost::system::error_code& error,
                                   std::size_t read_bytes);

    /*
     *  Hello
     */
    void read_server_name_length (const boost::system::error_code& error,
                                  std::size_t read_bytes);

    void read_full_server_name_hello (const boost::system::error_code& error,
                                  std::size_t read_bytes, size_t name_length);

    void read_hello_body_without_string (const boost::system::error_code& error,
                                 std::size_t read_bytes);

    /*
     * Accepted player
     */
    void read_player_id_and_name_length(const boost::system::error_code& error,
                                        std::size_t read_bytes,
                                        int num_repetitions,
                                        bool is_game_started);

    void read_player_name_string_and_address_length(
                                        const boost::system::error_code& error,
                                        std::size_t read_bytes,
                                        int num_repetitions,
                                        player_id_dt player_id,
                                        size_t name_length,
                                        bool is_game_started);

    void read_player_address_string_add_player(
                                        const boost::system::error_code& error,
                                        std::size_t read_bytes,
                                        int num_repetitions,
                                        player_id_dt player_id,
                                        size_t address_length,
                                        const string& player_name_temp,
                                        bool is_game_started);

    /*
     *  Game started
     */
    void read_player_map_length (const boost::system::error_code& error,
                                 std::size_t read_bytes);

    /*
     * Turn
     */
    void read_turn_and_list_length (const boost::system::error_code& error,
                                    std::size_t read_bytes);

    /*
     *  Events
     *
     *  `Num_repetitions` describes the number of the left elements in Events array.
     */
    void read_event_id (const boost::system::error_code& error,
                                    std::size_t read_bytes, map_list_length_dt
                                    num_repetitions);

    void update_after_turn();

    /*
    * Bomb placed
    */
    void read_bomb_id_and_position (const boost::system::error_code& error,
                        std::size_t read_bytes, map_list_length_dt
                        num_repetitions);

    /*
     *  Bomb exploded
     */
    void read_bomb_id_and_player_id_length (const boost::system::error_code& error,
                                            std::size_t read_bytes,
                                            map_list_length_dt
                                            num_repetitions);
    /*
     *  Bomb exploded - player id list
     *
     *  `player_id_left_elements` describes the number of the left elements
     *  of the partial array to read
     */
    void read_player_id (const boost::system::error_code& error,
                         std::size_t read_bytes, map_list_length_dt
                         num_repetitions,
                         map_list_length_dt player_id_left_elements,
                         bomb_id_dt bomb_id);

    /*
     *  Bomb exploded - blocks destroyed
     */
    void read_blocks_destroyed_length (const boost::system::error_code& error,
                                       std::size_t read_bytes,
                                       map_list_length_dt
                                       num_repetitions,
                                       bomb_id_dt bomb_id);

    void read_blocks_destroyed_positions (const boost::system::error_code& error,
                              std::size_t read_bytes,
                              map_list_length_dt
                              num_repetitions,
                              map_list_length_dt blocks_position_left_elements,
                              bomb_id_dt bomb_id);
    /*
     *  Player moved
     */
    void read_player_id_and_position (const boost::system::error_code& error,
                                      std::size_t read_bytes, map_list_length_dt
                                      num_repetitions);

    /*
     *  Block placed
     */
    void read_block_position (const boost::system::error_code& error,
                              std::size_t read_bytes, map_list_length_dt
                              num_repetitions);

    /************* End of Turn *************/

    /*
     *  Game ended
     */
    void read_score_map_length (const boost::system::error_code& error,
                                std::size_t read_bytes) ;

    void read_player_id_score (const boost::system::error_code& error,
                              std::size_t read_bytes, map_list_length_dt
                              num_repetitions);
    /******* End of reading server messages ******/

    void write_map_to_buffer(size_t& bytes_to_send, int map_type);

    void write_list_to_buffer(size_t& bytes_to_send, int list_type);

    size_t create_udp_message(bool is_Lobby);

    void process_data_from_server_send_to_gui();

    void after_send_to_gui([[maybe_unused]] const boost::system::error_code& error,
                           [[maybe_unused]] std::size_t sent_bytes);
public:
    Client_bomberman(io_context& io, const string& server_name,
                     const string& server_port, const string& gui_name,
                     const string& gui_port, uint16_t client_port,
                     string player_name);
};


#endif //CLIENT_BOMBERMAN_CONNECTION_UTILS_H
