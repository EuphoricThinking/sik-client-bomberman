//
// Created by heheszek on 22.05.22.
//

#ifndef CLIENT_BOMBERMAN_CONSTANTS_H
#define CLIENT_BOMBERMAN_CONSTANTS_H

const int client_number_arguments = 4;

const int u16_max =  65535;
const int u16_min = 0;

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
