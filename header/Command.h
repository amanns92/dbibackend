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
        // Constant definition
        const uint32_t BUFFER_SEGMENT_DATA_SIZE = 0x100000;

    public:
        //Command();
        //~Command();
        static void process_exit_command(UsbContext& context);
        static std::map<std::string, std::string> process_list_command(UsbContext& context, const std::string& work_dir_path);
        // Command polling loop
        static void poll_commands(UsbContext& context, const std::string& work_dir_path);

        static std::unique_ptr<UsbContext> connect_to_switch();
};



#endif //COMMAND_H
