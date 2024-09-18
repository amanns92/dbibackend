//
// Created by Sebastian Amann on 17.09.24.
//

#include <iostream>
#include <cstring>  // for std::memcpy
#include <vector>
#include <thread>      // for sleep_for
#include <chrono>      // for seconds
#include <filesystem>
#include "../header/UsbContext.h"
#include "../header/Command.h"
#include "../enums/CommandId.h"
#include "../enums/CommandType.h"

// Exit command processing
void Command::process_exit_command(UsbContext& context) {
    std::cout << "Exit" << std::endl;
    std::vector<uint8_t> buffer(16);
    std::memcpy(buffer.data(), "DBI0", 4);  // Magic string 'DBI0'
    constexpr auto cmd_type = static_cast<uint32_t>(CommandType::RESPONSE);
    constexpr auto cmd_id = static_cast<uint32_t>(CommandID::EXIT);
    constexpr auto data_size = 0;
    std::memcpy(buffer.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(buffer.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(buffer.data() + 12, &data_size, sizeof(data_size));
    context.write(buffer.data(), buffer.size());
    exit(0);  // Exit after processing
}

// List command processing
std::map<std::string, std::string> Command::process_list_command(UsbContext& context, const std::string& work_dir_path) {
    std::cout << "Get list" << std::endl;
    std::map<std::string, std::string> cached_titles;

    // Traverse directory and collect files
    for (const auto& entry : std::filesystem::recursive_directory_iterator(work_dir_path)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.size() >= 4) {
                std::string extension = filename.substr(filename.size() - 4);
                if (extension == ".nsp" || extension == ".nsz" || extension == ".xci") {
                    std::cout << "\tFound file: " << filename << std::endl;
                    cached_titles[filename] = entry.path().string();
                }
            }
        }
    }

    // Prepare list of file names
    std::string nsp_path_list;
    for (const auto& title : cached_titles) {
        nsp_path_list += title.first + "\n";
    }

    std::vector<uint8_t> nsp_path_list_bytes(nsp_path_list.begin(), nsp_path_list.end());
    uint32_t nsp_path_list_len = nsp_path_list_bytes.size();

    // Send header
    std::vector<uint8_t> buffer(16);
    std::memcpy(buffer.data(), "DBI0", 4);  // Magic string 'DBI0'
    constexpr auto  cmd_type = static_cast<uint32_t>(CommandType::RESPONSE);
    constexpr auto  cmd_id = static_cast<uint32_t>(CommandID::LIST);
    std::memcpy(buffer.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(buffer.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(buffer.data() + 12, &nsp_path_list_len, sizeof(nsp_path_list_len));
    context.write(buffer.data(), buffer.size());

    // Read acknowledgment
    std::vector<uint8_t> ack(16);
    context.read(ack.data(), ack.size(), 0);

    uint32_t ack_cmd_type = *reinterpret_cast<uint32_t*>(&ack[4]);
    uint32_t ack_cmd_id = *reinterpret_cast<uint32_t*>(&ack[8]);
    uint32_t ack_data_size = *reinterpret_cast<uint32_t*>(&ack[12]);

    std::cout << "Cmd Type: " << ack_cmd_type
              << ", Command ID: " << ack_cmd_id
              << ", Data Size: " << ack_data_size << std::endl;
    std::cout << "Ack received" << std::endl;

    // Send file names list
    context.write(nsp_path_list_bytes.data(), nsp_path_list_bytes.size());

    return cached_titles;
}

void Command::poll_commands(UsbContext& context, const std::string& work_dir_path) {
    std::cout << "Entering command loop" << std::endl;
    std::map<std::string, std::string> cmd_cache;

    while (true) {
        std::vector<uint8_t> cmd_header(16);
        int transferred = context.read(cmd_header.data(), 16, 0);
        if (transferred != 16) continue;

        if (std::memcmp(cmd_header.data(), "DBI0", 4) != 0) continue;

        uint32_t cmd_type = *reinterpret_cast<uint32_t*>(&cmd_header[4]);
        uint32_t cmd_id = *reinterpret_cast<uint32_t*>(&cmd_header[8]);
        uint32_t data_size = *reinterpret_cast<uint32_t*>(&cmd_header[12]);

        std::cout << "Cmd Type: " << cmd_type
                  << ", Command ID: " << cmd_id
                  << ", Data Size: " << data_size << std::endl;

        if (cmd_id == static_cast<uint32_t>(CommandID::EXIT)) {
            process_exit_command(context);
            break;
        } else if (cmd_id == static_cast<uint32_t>(CommandID::LIST)) {
            cmd_cache = process_list_command(context, work_dir_path);
        } else {
            std::cerr << "Unknown command ID: " << cmd_id << std::endl;
            process_exit_command(context);
        }
    }
}

std::unique_ptr<UsbContext> Command::connect_to_switch() {
    std::unique_ptr<UsbContext> switch_context;
    while (true) {
        try {
            switch_context = std::make_unique<UsbContext>(0x057E, 0x3000);
            break;
        } catch (const std::runtime_error& e) {
            std::cout << "Waiting for switch..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return switch_context;
}
