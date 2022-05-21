#include <iostream>
#include <boost/program_options.hpp>

using std::string;
using std::cout;
using std::endl;
using std::string;

namespace po = boost::program_options;
const int number_arguments = 4;

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
}

void print_saved_arguments(string& temp_gui, string& player_name,
                           uint16_t& port, string& server_address) {
    cout << "R gui: " << temp_gui << "\n"
    << "R plname: " << player_name << "\n"
    << "R port: " << port << "\n"
    << "R server: " << server_address << endl;
}

void read_command_line_options(string& temp_gui, string& player_name,
                               uint16_t& port, string& server_address,
                               int argc, char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "me please")
            ("gui-address,d", "host_name:port OR IPv4:port "
                              "OR IPv6:port\nGUI address")
            ("player-name,n", "Name of the player")
            ("port,p", po::value<uint16_t>(),
                    "The number of the port on which the client listens "
                    "messages from GUI")
            ("server-address,s", "host_name:port OR IPv4:port "
                                 "OR IPv6:port\nGUI address\nserver address")
    ;

    po::variables_map chosen_options;
    po::store(po::parse_command_line(argc, argv, desc), chosen_options);
    po::notify(chosen_options);

    if (chosen_options.count("help")) {
        cout << desc << endl;
    }
    else if (chosen_options.size() < number_arguments) {
        cout << "Incorrect number of arguments\n\n" << desc << endl;

        exit(1);
    }
    else {
        iterate_over_command_line_options(chosen_options);

        temp_gui = chosen_options["gui-address"].as<string>();
        player_name = chosen_options["player-name"].as<string>();
        port = chosen_options["port"].as<uint16_t>();
        server_address = chosen_options["server-address"].as<string>();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Hello, World!" << std::endl;
    string temp_gui;
    string player_name;
    uint16_t port;
    string server_address;

    read_command_line_options(temp_gui, player_name, port, server_address,
                              argc, argv);
    print_saved_arguments(temp_gui, player_name, port, server_address);

    return 0;
}
