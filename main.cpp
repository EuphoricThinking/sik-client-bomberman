#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include "constants.h"
#include "connection_utils.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::exception;
using std::strtoll;

using boost::asio::io_context;

using std::size_t;

namespace po = boost::program_options;

void validate_input(string& temp_gui,
                    int64_t port, string& server_address) {
    if (std::count(temp_gui.begin(), temp_gui.end(), ':') < 1
        || std::count(server_address.begin(), server_address.end(), ':') < 1) {
            cerr << "Incorrect address format\nNeeds: <host_identificator>:<port>\n";

            exit(1);
    }
    else if (port < u16_min || port > u16_max) {
        cerr << "Incorrect port value; should be in range [" << u16_min << ", "
             << u16_max << "]\n";
        exit(1);
    }
}

void read_command_line_options(string& temp_gui, string& player_name,
                               int64_t& port, string& server_address,
                               int argc, char* argv[]) {
    try {
        po::options_description desc("Allowed options");
        desc.add_options()
                ("help,h", "me please")
                ("gui-address,d", po::value<string>(),
                 "host_name:port OR IPv4:port OR IPv6:port\nGUI address")
                ("player-name,n", po::value<string>(), "Name of the player")
                ("port,p", po::value<int64_t>(),
                 "The number of the port on which the client listens "
                 "messages from GUI")
                ("server-address,s", po::value<string>(),
                 "host_name:port OR IPv4:port OR IPv6:port\nGUI "
                 "address\nserver address");

        po::variables_map chosen_options;
        po::store(po::parse_command_line(argc, argv, desc), chosen_options);
        po::notify(chosen_options);

        if (chosen_options.count("help")) {
            cout << desc << endl;

            exit(0);
        } else if (chosen_options.size() < client_number_arguments) {
            cerr << "Incorrect number of arguments\n\n" << desc << endl;

            exit(1);
        } else {
            temp_gui = chosen_options["gui-address"].as<string>();
            player_name = chosen_options["player-name"].as<string>();
            port = chosen_options["port"].as<int64_t>();
            server_address = chosen_options["server-address"].as<string>();
        }

        validate_input(temp_gui, port, server_address);
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";

        exit(1);
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";

        exit(1);
    }
}

void check_if_alphanumeric(const string& to_be_checked, size_t start) {
    for (size_t i = start; i < to_be_checked.length(); i++) {
        if (!std::isdigit(to_be_checked[i])) {
            cerr << "Port number should include only digits\n";

            exit(1);
        }
    }
}

void split_into_host_port(const string& to_be_splitted, string& host_name, string& host_port) {
    size_t position_of_last_colon = to_be_splitted.rfind(host_port_delimiter);
    size_t string_length = to_be_splitted.length();

    if (position_of_last_colon == std::string::npos || position_of_last_colon == string_length - 1) {
        cerr << "Incorrect format\nExpected: <host_name>:<port>\n";

        exit(1);
    }

    check_if_alphanumeric(to_be_splitted, position_of_last_colon + 1);

    host_name = to_be_splitted.substr(0, position_of_last_colon);
    host_port = to_be_splitted.substr(position_of_last_colon + 1, string_length - position_of_last_colon);
}

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
