#include "cli.hpp"

#include <cstdlib>
#include <iostream>
#include <print>

auto main(int argc, char* argv[]) -> int
{
    try {
        auto cli_result =
          typetrace::backend::Cli::create(std::span<char*>(argv, static_cast<std::size_t>(argc)));

        if (!cli_result) {
            std::println(std::cerr, "Failed to initialize CLI: {}", cli_result.error().message);
            return EXIT_FAILURE;
        }

        auto& cli = cli_result.value();
        auto run_result = cli.run();

        if (!run_result) {
            std::println(std::cerr, "Runtime error: {}", run_result.error().message);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::println(std::cerr, "Fatal error: {}", e.what());
        return EXIT_FAILURE;
    }
}
