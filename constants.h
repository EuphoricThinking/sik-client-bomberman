//
// Created by heheszek on 22.05.22.
//

#ifndef CLIENT_BOMBERMAN_CONSTANTS_H
#define CLIENT_BOMBERMAN_CONSTANTS_H

const int client_number_arguments = 4;

const int u16_max =  65535;
const int u16_min = 0;
const char host_port_delimiter = ':';

/*
 *  Server message parts lengths in  bytes
 */
const size_t hello_body_length_without_string = 11;
const size_t player_id_name_header_length = 2; // Player id and string length
const size_t player_id_bytes = 1;
const size_t map_list_length = 4;
const size_t string_max_length = 255;
const size_t string_length_info = 1;
const size_t turn_header = 6; // turn, list length
const size_t inner_event_header = 8;
const size_t player_id_score = 5;
const size_t bomb_pos_timer = 6;

// dt -> data type, defined type
using hello_lobby_game_exc_plc_dt = uint16_t;
using players_count_dt = uint8_t;
using player_id_dt = uint8_t;
using score_dt = uint32_t;
using position_dt = uint16_t;
using bombId_dt = uint32_t;
using timer_dt = uint16_t;

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

#endif //CLIENT_BOMBERMAN_CONSTANTS_H
