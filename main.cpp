#include <iostream>
#include <boost/program_options.hpp>
#include "connection_utils.h"
#include "input_utils.h"



int main(int argc, char* argv[]) {
    string temp_gui;
    string player_name;
    int64_t port;
    string temp_server_address;

    string host_gui_name;
    string port_gui;

    string server_name;
    string port_server;

    read_command_line_options(temp_gui, player_name, port, temp_server_address,
                              argc, argv);

    split_into_host_port(temp_gui, host_gui_name, port_gui);
    split_into_host_port(temp_server_address, server_name, port_server);

    io_context io;
    Client_bomberman client(io, server_name, port_server, host_gui_name,
                            port_gui, (uint16_t)port, player_name);
    io.run();

    return 0;
}
