#include <iostream>
#include <thread>      // for sleep_for
#include <map>
#include "Command.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <titles_directory>" << std::endl;
        return 1;
    }

    const std::string titles_dir = argv[1];

    Command command;

    try {
        auto usb_context = command.connect_to_switch();
        command.poll_commands(*usb_context, titles_dir);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
