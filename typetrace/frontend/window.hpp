#ifndef TYPETRACE_FRONTEND_WINDOW_HPP
#define TYPETRACE_FRONTEND_WINDOW_HPP

#include "models/keystroke_store.hpp"
#include "views/heatmap_view.hpp"
#include "views/statistics_view.hpp"
#include "views/verbose_view.hpp"

#include <giomm/settings.h>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/stack.h>
#include <memory>

namespace typetrace::frontend {

class Window final : public Gtk::ApplicationWindow
{
  public:
    [[nodiscard]]
    static auto create(Gtk::Application& app, std::shared_ptr<KeystrokeStore> store) -> Window*;

    ~Window() override = default;

    // Delete copy and move
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = delete;

  private:
    Window(Gtk::Application& app, std::shared_ptr<KeystrokeStore> store);

    auto setup_ui() -> void;
    auto setup_views() -> void;
    auto setup_signals() -> void;

    // Signal handlers
    auto on_backend_toggle_clicked() -> void;

    // Member variables
    std::shared_ptr<KeystrokeStore> keystroke_store_;
    Glib::RefPtr<Gio::Settings> settings_;

    // UI widgets (managed by GTKmm)
    Gtk::Stack* stack_{nullptr};
    Gtk::Button* backend_toggle_{nullptr};

    // State
    bool is_backend_running_{false};
};

} // namespace typetrace::frontend

#endif // TYPETRACE_FRONTEND_WINDOW_HPP
