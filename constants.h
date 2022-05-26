//
// Created by heheszek on 22.05.22.
//

#ifndef CLIENT_BOMBERMAN_CONSTANTS_H
#define CLIENT_BOMBERMAN_CONSTANTS_H

const int client_number_arguments = 4;

const int u16_max =  65535;
const int u16_min = 0;
const char host_port_delimiter = ':';

const size_t max_udp_roundup = 65550; //65527;
const size_t max_input_mess_roundup = 5;
const size_t tcp_buff_default = 300; // max string length and roundup

const uint8_t def_no_message = 10;
const uint8_t max_server_mess_id = 4;
const uint8_t max_event_mess_id = 3;

const uint8_t max_gui_message_id = 2;

const size_t max_input_message_bytes = 2;

/*
 *  Server message parts lengths in  bytes
 */
const size_t message_event_id_bytes = 1;
const size_t hello_body_length_without_string = 11;
const size_t player_id_name_header_length = 2; // Player id and string length
const size_t player_id_bytes = 1;
const size_t map_list_length = 4;
const size_t string_max_length = 255;
const size_t string_length_info = 1;
const size_t turn_header = 6; // turn, list length
const size_t inner_event_header = 8;
const size_t player_id_score_bytes = 5;
const size_t bomb_pos_timer = 6;
const size_t bomb_placed_header = 8;
const size_t bomb_id_list_length_header = 8;
const size_t player_id_pos_header = 5;
const size_t position_bytes = 4;
const size_t bomb_id_bytes = 4;
const size_t turn_bytes = 2;
const size_t lobby_exc_pc_bytes = 2;
const size_t score_bytes = 4;
const size_t single_position_bytes = 2;
const size_t timer_bytes = 2;

// dt -> data type, defined type
using hello_lobby_game_exc_plc_dt = uint16_t;
using players_count_dt = uint8_t;
using player_id_dt = uint8_t;
using score_dt = uint32_t;
using position_dt = uint16_t;
using bomb_id_dt = uint32_t;
using timer_dt = uint16_t;
using map_list_length_dt = uint32_t;

using pos_x = uint16_t;
using pos_y = uint16_t;

using id_dt = uint8_t;
using direction_dt = uint8_t;

/*
 *  Client -> Server
 */

enum ClientMessage {
    Join,           // { name: String }
    PlaceBomb,
    PlaceBlock,
    Move            // { direction: Direction }
};

enum Direction {
    Up,
    Right,
    Down,
    Left,
};

/*
 *  Server -> Client
 */

enum ServerMessage {
    Hello,
                    /*
                    server_name: String,
                    players_count: u8,
                    size_x: u16,
                    size_y: u16,
                    game_length: u16,
                    explosion_radius: u16,
                    bomb_timer: u16,
                    */
    AcceptedPlayer,
                    /*
                    id: PlayerId,
                    player: Player,
                    */
    GameStarted,
                    /*
                    players: Map<PlayerId, Player>,
                    */
    Turn,
                    /*
                    turn: u16,
                    events: List<Event>,
                    */
    GameEnded
                    /*
                    scores: Map<PlayerId, Score>,
                    */
};

/*
 *  Client -> GUI
 */

enum DrawMessage {
    Lobby,
                    /*
                    server_name: String,
                    players_count: u8,
                    size_x: u16,
                    size_y: u16,
                    game_length: u16,
                    explosion_radius: u16,
                    bomb_timer: u16,
                    players: Map<PlayerId, Player>
                    */
    Game
                    /*
                    server_name: String,
                    size_x: u16,
                    size_y: u16,
                    game_length: u16,
                    turn: u16,
                    players: Map<PlayerId, Player>,
                    player_positions: Map<PlayerId, Position>,
                    blocks: List<Position>,
                    bombs: List<Bomb>,
                    explosions: List<Position>,
                    scores: Map<PlayerId, Score>,
                    */
};

/*
 *  GUI -> Client
 */

enum InputMessage {
    PlaceBombGUI,
    PlaceBlockGUI,
    MoveGUI        // { direction: Direction }
};

enum Events {
    BombPlaced, // { id: BombId, position: Position },
    BombExploded, //{ id: BombId, robots_destroyed: List<PlayerId>, blocks_destroyed: List<Position> },
    PlayerMoved,  // { id: PlayerId, position: Position },
    BlockPlaced // { position: Position },
};

#endif //CLIENT_BOMBERMAN_CONSTANTS_H
