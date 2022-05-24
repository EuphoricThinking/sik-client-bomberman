#include <iostream>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include "constants.h"

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::exception;
using std::strtoll;

using std::size_t;

namespace po = boost::program_options;

void iterate_over_command_line_options(po::variables_map chosen_options) {
    for (const auto& it : chosen_options) {
        cout << it.first.c_str() << " ";
        auto& value = it.second.value();
        if (auto v = boost::any_cast<int64_t>(&value))
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
        exit(1);
    }
}
//
//class u16 {
//public:
//    u16(int64_t i): i(i) {}
//    int64_t i;
//};
//
//void validate(boost::any& v,
//              const std::vector<std::string>& values,
//              u16* target_type, int)
//{
//
//    using namespace boost::program_options;
//
//    // Make sure no previous assignment to 'a' was made.
//    validators::check_first_occurrence(v);
//    // Extract the first string from 'values'. If there is more than
//    // one string, it's an error, and exception will be thrown.
//    const string& s = validators::get_single_string(values);
//    int64_t to_int = strtoll(s.c_str(), NULL, 10);
//    if (to_int >= 0 && to_int <= u16_max) {
//        v = boost::any(u16(to_int));
//    } else {
//        throw validation_error(validation_error::invalid_option_value);
//    }
//}


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

void check_if_alphanumeric(const string& to_be_checked, size_t start) {
    for (int i = start; i < to_be_checked.length(); i++) {
        if (!std::isalpha(to_be_checked[i])) {
            cerr << "Port number should include only digits\n";

            exit(1);
        }
    }
}
void split_into_host_port(string& host_name, string& host_port, const string& to_be_splitted) {
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
//    std::cout << "Hello, World!" << std::endl;
    string temp_gui;
    string player_name;
    int64_t port;
    string server_address;

    string host_gui_name;
    string port_gui;

    string server_name;
    string port_server;

    read_command_line_options(temp_gui, player_name, port, server_address,
                              argc, argv);
    print_saved_arguments(temp_gui, player_name, port, server_address);

    split_into_host_port(temp_gui, host_gui_name, port_gui);
    split_into_host_port(server_address, server_name, port_server);

    return 0;
}
