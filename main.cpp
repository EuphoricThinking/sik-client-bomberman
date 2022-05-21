#include <iostream>
#include <boost/program_options.hpp>

using std::string;
using std::cout;
using std::endl;

namespace po = boost::program_options;

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
    
}
int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
