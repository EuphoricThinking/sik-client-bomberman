//
// Created by heheszek on 26.05.22.
//

#ifndef CLIENT_BOMBERMAN_INPUT_UTILS_H
#define CLIENT_BOMBERMAN_INPUT_UTILS_H

#include <iostream>

using std::string;

/*
 * Header for parsing command-line arguments
 */

void read_command_line_options(string& temp_gui, string& player_name,
                               int64_t& port, string& server_address,
                               int argc, char* argv[]);

// Splits string of <host>:<port> into two strings, saved in passed references
void split_into_host_port(const string& to_be_splitted, string& host_name,
                          string& host_port);

#endif //CLIENT_BOMBERMAN_INPUT_UTILS_H
