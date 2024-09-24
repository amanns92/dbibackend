//
// Created by Sebastian Amann on 17.09.24.
//

#include <iostream>
#include <cstring>  // for std::memcpy
#include <vector>
#include <thread>      // for sleep_for
#include <chrono>      // for seconds
#include <fstream>
#include <filesystem>
#include "../header/UsbContext.h"
#include "../header/Command.h"
#include "../enums/CommandId.h"
#include "../enums/CommandType.h"

Command::Command() : context(connect_to_switch()) {
    // Now context is a unique_ptr and the UsbContext's lifetime is managed
}

// Exit command processing
void Command::process_exit_command() {
    std::cout << "Exit" << std::endl;
    std::vector<uint8_t> buffer(16);
    std::memcpy(buffer.data(), "DBI0", 4);  // Magic string 'DBI0'
    constexpr auto cmd_type = static_cast<uint32_t>(CommandType::RESPONSE);
    constexpr auto cmd_id = static_cast<uint32_t>(CommandID::EXIT);
    constexpr auto data_size = 0;
    std::memcpy(buffer.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(buffer.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(buffer.data() + 12, &data_size, sizeof(data_size));
    context->write(buffer.data(), buffer.size());
    exit(0);  // Exit after processing
}

// List command processing
std::map<std::string, std::string> Command::process_list_command(std::string& work_dir_path) {
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

    const std::vector<uint8_t> nsp_path_list_bytes(nsp_path_list.begin(), nsp_path_list.end());
    uint32_t nsp_path_list_len = nsp_path_list_bytes.size();

    // Send header
    std::vector<uint8_t> buffer(16);
    std::memcpy(buffer.data(), "DBI0", 4);  // Magic string 'DBI0'
    constexpr auto  cmd_type = static_cast<uint32_t>(CommandType::RESPONSE);
    constexpr auto  cmd_id = static_cast<uint32_t>(CommandID::LIST);
    std::memcpy(buffer.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(buffer.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(buffer.data() + 12, &nsp_path_list_len, sizeof(nsp_path_list_len));
    context->write(buffer.data(), buffer.size());

    // Read acknowledgment
    std::vector<uint8_t> ack(16);
    context->read(ack.data(), ack.size(), 0);

    uint32_t ack_cmd_type = *reinterpret_cast<uint32_t*>(&ack[4]);
    uint32_t ack_cmd_id = *reinterpret_cast<uint32_t*>(&ack[8]);
    uint32_t ack_data_size = *reinterpret_cast<uint32_t*>(&ack[12]);

    std::cout << "Cmd Type: " << ack_cmd_type
              << ", Command ID: " << ack_cmd_id
              << ", Data Size: " << ack_data_size << std::endl;
    std::cout << "Ack received" << std::endl;

    // Send file names list
    context->write(nsp_path_list_bytes.data(), nsp_path_list_bytes.size());

    return cached_titles;
}

void Command::process_file_range_command(uint32_t data_size, std::string& work_dir_path, std::map<std::string, std::string>* cache) {
    std::cout << "File range" << std::endl;

    // Send command to context
    std::vector<uint8_t> write_buffer(16);
    std::memcpy(write_buffer.data(), "DBI0", 4);  // Magic string 'DBI0'
    uint32_t cmd_type = static_cast<uint32_t>(CommandType::ACK);
    uint32_t cmd_id = static_cast<uint32_t>(CommandID::FILE_RANGE);

    std::memcpy(write_buffer.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(write_buffer.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(write_buffer.data() + 12, &data_size, sizeof(data_size));

    context->write(write_buffer.data(), write_buffer.size());

    // Read file range header from context
    std::vector<uint8_t> file_range_header(data_size);
    context->read(file_range_header.data(), file_range_header.size(), 0);

    // Extract values from header
    uint32_t range_size = *reinterpret_cast<uint32_t*>(file_range_header.data());
    uint64_t range_offset = *reinterpret_cast<uint64_t*>(file_range_header.data() + 4);
    uint32_t nsp_name_len = *reinterpret_cast<uint32_t*>(file_range_header.data() + 12);

    // Extract nsp_name and process cache
    std::string nsp_name(file_range_header.begin() + 16, file_range_header.end());

    if (cache) {
        auto it = cache->find(nsp_name);
        if (it != cache->end()) {
            nsp_name = it->second;
        }
    }

    std::cout << "Range Size: " << range_size
              << ", Range Offset: " << range_offset
              << ", Name len: " << nsp_name_len
              << ", Name: " << nsp_name << std::endl;

    // Prepare response and send it
    std::vector<uint8_t> response_bytes(16);
    std::memcpy(response_bytes.data(), "DBI0", 4);  // Magic string 'DBI0'
    cmd_type = static_cast<uint32_t>(CommandType::RESPONSE);

    std::memcpy(response_bytes.data() + 4, &cmd_type, sizeof(cmd_type));
    std::memcpy(response_bytes.data() + 8, &cmd_id, sizeof(cmd_id));
    std::memcpy(response_bytes.data() + 12, &range_size, sizeof(range_size));

    context->write(response_bytes.data(), response_bytes.size());

    // Read acknowledgment from context
    std::vector<uint8_t> ack(16);
    context->read(ack.data(), ack.size(), 0);

    cmd_type = *reinterpret_cast<uint32_t*>(&ack[4]);
    cmd_id = *reinterpret_cast<uint32_t*>(&ack[8]);
    data_size = *reinterpret_cast<uint32_t*>(&ack[12]);

    std::cout << "Cmd Type: " << cmd_type
              << ", Command id: " << cmd_id
              << ", Data size: " << data_size << std::endl;
    std::cout << "Ack" << std::endl;

    std::string nsp_path = work_dir_path + nsp_name;
    if (!std::filesystem::exists(nsp_path)) {
        throw std::runtime_error("File does not exist: " + nsp_path);
    }

    // Open the file and write its contents to context
    std::ifstream file(nsp_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + nsp_name);
    }

    file.seekg(range_offset);

    uint64_t curr_off = 0;
    uint64_t end_off = range_size;
    uint32_t read_size = BUFFER_SEGMENT_DATA_SIZE;

    while (curr_off < end_off) {
        if (curr_off + read_size >= end_off) {
            read_size = static_cast<uint32_t>(end_off - curr_off);
        }

        // Read the data from the file
        std::vector<uint8_t> buf(read_size);
        file.read(reinterpret_cast<char*>(buf.data()), read_size);

        // Write the data to the context
        context->write(buf.data(), read_size);

        curr_off += read_size;
    }
}

void Command::poll_commands(std::string& work_dir_path) {
    std::cout << "Entering command loop" << std::endl;

    while (true) {
        std::vector<uint8_t> cmd_header(16);
        int transferred = context->read(cmd_header.data(), 16, 0);
        if (transferred != 16) continue;

        if (std::memcmp(cmd_header.data(), "DBI0", 4) != 0) continue;

        uint32_t cmd_type = *reinterpret_cast<uint32_t*>(&cmd_header[4]);
        uint32_t cmd_id = *reinterpret_cast<uint32_t*>(&cmd_header[8]);
        uint32_t data_size = *reinterpret_cast<uint32_t*>(&cmd_header[12]);

        std::cout << "Cmd Type: " << cmd_type
                  << ", Command ID: " << cmd_id
                  << ", Data Size: " << data_size << std::endl;

        if (cmd_id == static_cast<uint32_t>(CommandID::EXIT)) {
            process_exit_command();
            break;
        }
        std::map<std::string, std::string> cmd_cache;
        if (cmd_id == static_cast<uint32_t>(CommandID::LIST)) {
           cmd_cache = process_list_command(work_dir_path);
        } else if (cmd_id == static_cast<uint32_t>(CommandID::FILE_RANGE)) {
            process_file_range_command(data_size, work_dir_path, &cmd_cache);
        }else {
            std::cerr << "Unknown command ID: " << cmd_id << std::endl;
            process_exit_command();
        }
    }
}

std::unique_ptr<UsbContext> Command::connect_to_switch() {
    std::unique_ptr<UsbContext> switch_context;
    while (true) {
        try {
            switch_context = std::make_unique<UsbContext>(0x057E, 0x3000);
            break;
        } catch (const std::runtime_error&) {
            std::cout << "Waiting for switch..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return switch_context;
}
