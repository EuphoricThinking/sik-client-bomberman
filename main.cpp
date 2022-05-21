#include <iostream>
#include <boost/program_options.hpp>
#include "constants.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::exception;

namespace po = boost::program_options;

void iterate_over_command_line_options(po::variables_map chosen_options) {
    for (const auto& it : chosen_options) {
        cout << it.first.c_str() << " ";
        auto& value = it.second.value();
        if (auto v = boost::any_cast<uint32_t>(&value))
            std::cout << *v;
        else if (auto v1 = boost::any_cast<std::string>(&value))
            std::cout << *v1;
        else
            std::cout << "error";
    }

    cout << "\n";
}

void print_saved_arguments(string& temp_gui, string& player_name,
                           int64_t& port, string& server_address) {
    cout << "R gui: " << temp_gui << "\n"
    << "R plname: " << player_name << "\n"
    << "R port: " << port << "\n"
    << "R server: " << server_address << endl;
}

void validate_input(string& temp_gui,
                    int64_t port, string& server_address) {
    if (std::count(temp_gui.begin(), temp_gui.end(), ':') != 1
        || std::count(server_address.begin(), server_address.end(), ':') != 1) {
        cout << "Incorrect address format\nNeeds: <host_identificator>:<port>\n";

        exit(1);
    }
    else if (port < u16_min || port > u16_max) {
        cout << "Incorrect port value; should be in range [" << u16_min << ", "
             << u16_max << "]\n";
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
        cout << chosen_options.size() << " const: " << client_number_arguments
             << endl;

        if (chosen_options.count("help")) {
            cout << desc << endl;

            exit(0);
        } else if (chosen_options.size() < client_number_arguments) {
            cout << "Incorrect number of arguments\n\n" << desc << endl;

            exit(1);
        } else {
            iterate_over_command_line_options(chosen_options);

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

int main(int argc, char* argv[]) {
//    std::cout << "Hello, World!" << std::endl;
    string temp_gui;
    string player_name;
    int64_t port;
    string server_address;

    read_command_line_options(temp_gui, player_name, port, server_address,
                              argc, argv);
    print_saved_arguments(temp_gui, player_name, port, server_address);

    return 0;
}
