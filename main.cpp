#include <iostream>
#include <thread>      // for sleep_for
#include <map>
#include "header/Command.h"


int main(const int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <titles_directory>" << std::endl;
        return 1;
    }

    std::string titles_dir = argv[1];

    try {
        Command command = Command();
        command.poll_commands(titles_dir);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
