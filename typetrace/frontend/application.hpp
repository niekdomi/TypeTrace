#ifndef TYPETRACE_FRONTEND_APPLICATION_HPP
#define TYPETRACE_FRONTEND_APPLICATION_HPP

#include <gtkmm/aboutdialog.h>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <iostream>
#include <print>

namespace typetrace::frontend {

class Application final : public Gtk::Application
{
  public:
    [[nodiscard]]
    static auto create() -> Glib::RefPtr<Application>
    {
        return Glib::make_refptr_for_instance<Application>(new Application());
    }

    ~Application() override = default;

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    auto operator=(const Application&) -> Application& = delete;
    auto operator=(Application&&) -> Application& = delete;

  protected:
    Application() : Gtk::Application("edu.ost.typetrace", Gio::Application::Flags::DEFAULT_FLAGS) {}

    auto on_activate() -> void override
    {
        if (main_window_ == nullptr) {
            main_window_ = create_window();
        }
        main_window_->present();
    }

    auto on_startup() -> void override
    {
        Gtk::Application::on_startup();
        setup_actions();
    }

  private:
    auto create_window() -> Gtk::ApplicationWindow*
    {
        auto builder = Gtk::Builder::create_from_resource("/edu/ost/typetrace/ui/window.ui");

        auto* window = builder->get_widget<Gtk::ApplicationWindow>("TypeTraceWindow");
        if (window == nullptr) {
            throw std::runtime_error("Failed to load window from UI file");
        }

        add_window(*window);

        window->signal_hide().connect([this]() { main_window_ = nullptr; });

        return window;
    }

    auto setup_actions() -> void
    {
        add_action("about", [this]() { on_about_action(); });
        set_accel_for_action("app.quit", "<Ctrl>Q");
    }

    auto on_about_action() -> void
    {
        auto dialog = Gtk::AboutDialog();
        dialog.set_transient_for(*main_window_);
        dialog.set_modal(true);
        dialog.set_program_name("TypeTrace");
        dialog.set_version("0.1.0");
        dialog.set_comments("Track and visualize your keyboard usage");
        dialog.set_website("https://github.com/yourusername/typetrace");
        dialog.set_website_label("GitHub Repository");
        dialog.set_license_type(Gtk::License::GPL_3_0);

        dialog.present();
    }

    Gtk::ApplicationWindow* main_window_{nullptr};
};

} // namespace typetrace::frontend

#endif // TYPETRACE_FRONTEND_APPLICATION_HPP
