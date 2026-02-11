#pragma once

#include "database_manager.hpp"
#include "errors.hpp"
#include "event_handler.hpp"
#include "logger.hpp"
#include "types.hpp"
#include "version.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace typetrace::backend {

class Cli final
{
  public:
    /// Factory method to create a CLI instance
    [[nodiscard]]
    static auto create(std::span<const char* const> args) -> std::expected<Cli, Error>
    {
        Cli cli;
        parse_arguments(args);

        const auto db_dir = TRY(get_database_dir());
        auto db_mgr = TRY(DatabaseManager::create(db_dir));
        cli.db_manager_ = std::make_unique<DatabaseManager>(std::move(db_mgr));

        auto evt_handler = TRY(EventHandler::create());
        cli.event_handler_ = std::make_unique<EventHandler>(std::move(evt_handler));

        // Set up callback for EventHandler to flush buffer to database
        cli.event_handler_->set_buffer_callback(
          [&db_mgr_cb =
             cli.db_manager_](const std::vector<common::KeystrokeEvent>& buffer) -> void {
              if (const auto result = db_mgr_cb->write_to_database(buffer); !result) {
                  common::Logger::instance().error("Failed to write to database: {}",
                                                   result.error().message);
              }
          });

        return cli;
    }

    /// Runs the main event loop for keystroke tracing
    auto run() -> std::expected<void, Error>
    {
        while (true) { // TODO(domi): Use event handler to quit
            event_handler_->trace();
        }
    }

  private:
    Cli() = default;

    /// Parses and processes command line arguments
    static auto parse_arguments(std::span<const char* const> args) -> int
    {
        for (const auto i : std::views::iota(1U, args.size())) {
            const std::string_view arg = args[i];

            if (arg == "-h" || arg == "--help") {
                show_help(args[0]);
                return EXIT_SUCCESS;
            }
            if (arg == "-v" || arg == "--version") {
                show_version();
                return EXIT_SUCCESS;
            }
            if (arg == "-d" || arg == "--debug") {
                auto& logger = common::Logger::instance();
                logger.debug("Debug mode enabled");
            } else {
                std::println(std::cerr, "Unknown option: {}", arg);
                show_help(args[0]);
                return EXIT_FAILURE;
            }
        }

        return EXIT_SUCCESS;
    }

    /// Displays help information and usage instructions
    static auto show_help(std::string_view program_name) -> void
    {
        std::println(R"(
The backend of TypeTrace
Version: {}

Usage: {} [OPTION…]

Options:
 -h, --help      Display help then exit.
 -v, --version   Display version then exit.
 -d, --debug     Enable debug mode.

Warning: This is the backend and is not designed to run by users.
You should run the frontend of TypeTrace which will run this.
)",
                     PROJECT_VERSION,
                     program_name);
    }

    /// Displays the program version information
    static auto show_version() -> void
    {
        std::println("TypeTrace Backend v{}", PROJECT_VERSION);
    }

    /// Gets the database directory path using XDG or fallback locations
    [[nodiscard]]
    static auto get_database_dir() -> std::expected<std::filesystem::path, Error>
    {
        const char* home = std::getenv("HOME");
        if (home == nullptr) {
            return std::unexpected(make_system_error("HOME environment variable not set"));
        }

        auto db_dir = std::filesystem::path(home) / ".local" / "share" / PROJECT_NAME;

        try {
            if (!std::filesystem::exists(db_dir)) {
                std::filesystem::create_directories(db_dir);
                auto& logger = common::Logger::instance();
                logger.info("Created database directory: {}", db_dir.string());
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            return std::unexpected(
              make_system_error(std::format("Failed to create database directory: {}", e.what())));
        }

        return db_dir;
    }

    std::unique_ptr<EventHandler> event_handler_;
    std::unique_ptr<DatabaseManager> db_manager_;
};

} // namespace typetrace::backend
