//
// Created by Sebastian Amann on 17.09.24.
//

#ifndef COMMAND_H
#define COMMAND_H
#include <string>
#include <map>
#include "UsbContext.h"

class Command {
private:
    uint32_t BUFFER_SEGMENT_DATA_SIZE = 0x100000;
    std::unique_ptr<UsbContext> context; // Change to unique_ptr

public:
    Command();
    ~Command() = default; // Destructor is defaulted since it's not doing anything special.
    void process_exit_command();
    std::map<std::string, std::string> process_list_command(std::string& work_dir_path);
    void poll_commands(std::string& work_dir_path);
    void process_file_range_command(uint32_t data_size, std::string& work_dir_path, std::map<std::string, std::string>* cache = nullptr);

    std::unique_ptr<UsbContext> connect_to_switch();
};




#endif //COMMAND_H
