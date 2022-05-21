#include <iostream>
#include <boost/program_options.hpp>

using std::string;
using std::cout;
using std::endl;

namespace po = boost::program_options;
const int number_arguments = 4;

void iterate_over_command_line_options(po::variables_map chosen_options) {
    for (const auto& it : chosen_options) {
        cout << it.first.c_str() << " ";
        auto& value = it.second.value();
        if (auto v = boost::any_cast<uint32_t>(&value))
            std::cout << *v;
        else if (auto v = boost::any_cast<std::string>(&value))
            std::cout << *v;
        else
            std::cout << "error";
    }
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
    }
    else {
        iterate_over_command_line_options(chosen_options);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
