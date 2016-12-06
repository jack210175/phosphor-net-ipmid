#include "command_table.hpp"

#include <iomanip>
#include <iostream>

#include <host-ipmid/ipmid-api.h>
#include "message_handler.hpp"
#include "sessions_manager.hpp"

namespace command
{

void Table::registerCommand(CommandID inCommand, std::unique_ptr<Entry>&& entry)
{
    std::cout << "I> Registering Command" << std::hex
              << inCommand.command << "\n";

    commandTable[inCommand.command] = std::move(entry);
}

std::vector<uint8_t> Table::executeCommand(uint32_t inCommand,
                                           std::vector<uint8_t>& commandData,
                                           const message::Handler& handler)
{
    using namespace std::chrono_literals;

    std::vector<uint8_t> response;

    auto iterator = commandTable.find(inCommand);

    if (iterator == commandTable.end())
    {
        std::cerr << "E> Table:: Command Not found: 0x" << std::hex << inCommand
                  << "\n";

        response.resize(1);
        response[0] = IPMI_CC_INVALID;
    }
    else
    {
        auto start = std::chrono::steady_clock::now();

        response = iterator->second->executeCommand(commandData, handler);

        auto end = std::chrono::steady_clock::now();

        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>
                              (end - start);

        // If command time execution time exceeds 2 seconds, log a time
        // exceeded message
        if (elapsedSeconds > 2s)
        {
            std::cerr << "E> IPMI command timed out:Elapsed time = "
                      << elapsedSeconds.count() << "s" << "\n";
        }
    }
    return response;
}

std::vector<uint8_t> NetIpmidEntry::executeCommand(
        std::vector<uint8_t>& commandData,
        const message::Handler& handler)
{
    std::vector<uint8_t> errResponse;

    // Check if the command qualifies to be run prior to establishing a session
    if (!sessionless && (handler.sessionID == session::SESSION_ZERO))
    {
        errResponse.resize(1);
        errResponse[0] = IPMI_CC_INSUFFICIENT_PRIVILEGE;
        std::cerr << "E> Table::Not enough privileges for command 0x"
                  << std::hex << command.command << "\n";
        return errResponse;
    }

    return functor(commandData, handler);
}

} // namespace command