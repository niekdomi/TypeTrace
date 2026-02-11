#pragma once

#include "constants.hpp"
#include "errors.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "types.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <expected>
#include <fcntl.h>
#include <format>
#include <functional>
#include <grp.h>
#include <iostream>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libinput.h>
#include <libudev.h>
#include <linux/input-event-codes.h>
#include <memory>
#include <optional>
#include <poll.h>
#include <print>
#include <unistd.h>
#include <vector>

namespace typetrace::backend {

using Clock = std::chrono::steady_clock;

class EventHandler
{
  public:
    /// Factory method to create an EventHandler instance
    [[nodiscard]]
    static auto create() -> std::expected<EventHandler, Error>
    {
        EventHandler handler;

        TRY(handler.check_input_group_membership());
        TRY(handler.initialize_libinput());
        TRY(handler.check_device_accessibility());

        return handler;
    }

    /// Sets the callback function to be called when the buffer needs to be flushed
    auto set_buffer_callback(std::function<void(const std::vector<common::KeystrokeEvent>&)> callback) -> void
    {
        buffer_callback_ = std::move(callback);
    }

    /// Traces keyboard events and processes them into keystroke events
    auto trace() -> void
    {
        struct pollfd pfd = {
          .fd = libinput_get_fd(li_.get()),
          .events = POLLIN,
          .revents = 0,
        };

        const int result = poll(&pfd, 1, POLL_TIMEOUT_MS);

        if (result < 0) {
            common::Logger::instance().error("Poll failed with error: {}", std::strerror(errno));
            return;
        }

        if (result > 0 && ((POLLIN & pfd.revents) != 0)) {
            libinput_dispatch(li_.get());

            // Process all available events
            struct libinput_event* event = nullptr;
            while ((event = libinput_get_event(li_.get())) != nullptr) {
                if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
                    if (const auto keystroke = process_keyboard_event(event)) {
                        buffer_.push_back(*keystroke);
                    }
                }

                libinput_event_destroy(event);
            }
        }

        if (should_flush()) {
            flush_buffer();
        }
    }

  private:
    /// Private constructor - use create() factory method
    EventHandler() = default;

    /// Checks if the current user is a member of the 'input' group
    [[nodiscard]]
    static auto check_input_group_membership() -> std::expected<void, Error>
    {
        common::Logger::instance().info("Checking for 'input' group membership...");

        struct group const * const input_group = getgrnam("input");
        if (input_group == nullptr) {
            return std::unexpected(make_system_error("Input group does not exist. Please create it"));
        }

        const gid_t input_gid = input_group->gr_gid;

        const int ngroups = getgroups(0, nullptr);
        std::vector<gid_t> groups(static_cast<std::size_t>(ngroups));
        getgroups(ngroups, groups.data());

        if (std::ranges::find(groups, input_gid) == groups.end()) {
            print_input_group_permission_help();
            return std::unexpected(make_permission_error("User not in 'input' group. See instructions above"));
        }

        common::Logger::instance().info("User is a member of the 'input' group");
        return {};
    }

    /// Prints help information for input group permission issues
    static auto print_input_group_permission_help() -> void
    {
        std::println(std::cerr, R"(
===================== Permission Error =====================
TypeTrace requires access to input devices to function.

To grant access, add your user to the 'input' group:
    sudo usermod -a -G input $USER

Then log out and log back in for the changes to take effect.
============================================================
)");
    }

    /// Checks if input devices are accessible and functional
    [[nodiscard]]
    auto check_device_accessibility() const -> std::expected<void, Error>
    {
        common::Logger::instance().info("Checking for device accessibility...");

        if (li_ == nullptr) {
            return std::unexpected(make_system_error("Libinput is not initialized. Cannot check device accessibility"));
        }

        if (libinput_dispatch(li_.get()) < 0) {
            return std::unexpected(make_system_error("Failed to dispatch libinput events"));
        }

        struct libinput_event* event = libinput_get_event(li_.get());
        if ((event == nullptr) || libinput_event_get_type(event) != LIBINPUT_EVENT_DEVICE_ADDED) {
            if (event != nullptr) {
                libinput_event_destroy(event);
            }
            return std::unexpected(make_system_error("No input devices found or not accessible"));
        }

        common::Logger::instance().info("Input devices are accessible");
        libinput_event_destroy(event);
        return {};
    }

    /// Initializes libinput context and assigns seat
    [[nodiscard]]
    auto initialize_libinput() -> std::expected<void, Error>
    {
        common::Logger::instance().info("Initializing libinput context...");

        static const struct libinput_interface interface = {
          .open_restricted = [](const char* const path, const int flags, void*) -> int { return ::open(path, flags); },
          .close_restricted = [](const int fd, void*) -> void { ::close(fd); },
        };

        // Initialize udev
        udev_.reset(udev_new());
        if (udev_ == nullptr) {
            return std::unexpected(make_system_error("Failed to initialize udev"));
        }

        // Initialize libinput
        li_.reset(libinput_udev_create_context(&interface, nullptr, udev_.get()));
        if (li_ == nullptr) {
            return std::unexpected(make_system_error("Failed to initialize libinput from udev"));
        }

        // Assign seat0
        if (libinput_udev_assign_seat(li_.get(), "seat0") < 0) {
            return std::unexpected(make_system_error("Failed to assign seat to libinput"));
        }

        common::Logger::instance().info("Libinput initialized successfully");
        return {};
    }

    /// Processes a libinput keyboard event into a keystroke event
    [[nodiscard]]
    auto process_keyboard_event(struct libinput_event* event) -> std::optional<common::KeystrokeEvent>
    {
        cauto& logger = common::Logger::instance();

        auto* keyboard_event = libinput_event_get_keyboard_event(event);
        if (keyboard_event == nullptr) {
            logger.warn("Failed to get keyboard event from libinput event");
            return std::nullopt;
        }

        // Ignore releases, only process key presses
        if (libinput_event_keyboard_get_key_state(keyboard_event) != LIBINPUT_KEY_STATE_PRESSED) {
            return std::nullopt;
        }

        const auto key_code = libinput_event_keyboard_get_key(keyboard_event);
        const auto* key_name_str = libevdev_event_code_get_name(EV_KEY, static_cast<unsigned int>(key_code));
        const auto key_name = key_name_str != nullptr ? std::string_view(key_name_str) : std::string_view("UNKNOWN");
        // const auto time_now = std::chrono::system_clock::now();

        common::KeystrokeEvent keystroke{.key_name = key_name,
                                         .date = "FIX_ME", // std::format("{:%Y-%m-%d}",
                                         .key_code = key_code,
                                         // std::chrono::time_point_cast<std::chrono::days>(time_now)),
                                         .count = 1};

        logger.debug("Added keystroke [{}/{}] to buffer: {} (code: {})",
                     buffer_.size() + 1,
                     BUFFER_SIZE,
                     keystroke.key_name.data(),
                     key_code);

        return keystroke;
    }

    /// Determines if the buffer should be flushed based on size and time
    [[nodiscard]]
    auto should_flush() const -> bool
    {
        if (buffer_.size() >= BUFFER_SIZE) {
            common::Logger::instance().debug("Flushing buffer: size threshold reached ({} events)", buffer_.size());
            return true;
        }

        if (!buffer_.empty()) {
            const auto elapsed_duration = Clock::now() - last_flush_time_;

            if (elapsed_duration >= std::chrono::seconds(BUFFER_TIMEOUT)) {
                common::Logger::instance().debug("Flushing buffer: time threshold reached ({}s elapsed)",
                                                 BUFFER_TIMEOUT);
                return true;
            }
        }

        return false;
    }

    /// Flushes the current buffer by calling the buffer callback
    auto flush_buffer() -> void
    {
        if (buffer_.empty()) {
            return;
        }

        if (buffer_callback_) {
            const auto elapsed_seconds =
              std::chrono::duration_cast<std::chrono::duration<double>>(Clock::now() - last_flush_time_).count();
            common::Logger::instance().debug(
              "Flushing buffer with {} events in {:.2f}s to database", buffer_.size(), elapsed_seconds);

            buffer_callback_(buffer_);
        }

        buffer_.clear();
        last_flush_time_ = Clock::now();
    }

    std::vector<common::KeystrokeEvent> buffer_;
    Clock::time_point last_flush_time_;

    std::function<void(const std::vector<common::KeystrokeEvent>&)> buffer_callback_;

    std::unique_ptr<struct libinput, decltype(&libinput_unref)> li_{nullptr, &libinput_unref};
    std::unique_ptr<struct udev, decltype(&udev_unref)> udev_{nullptr, &udev_unref};
};

} // namespace typetrace::backend
