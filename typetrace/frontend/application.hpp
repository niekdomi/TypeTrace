#ifndef TYPETRACE_FRONTEND_APPLICATION_HPP
#define TYPETRACE_FRONTEND_APPLICATION_HPP

#include <gtkmm/button.h>
#include <gtkmm/window.h>
#include <print>
#include <sigc++-3.0/sigc++/functors/mem_fun.h>

namespace typetrace::frontend {

class Application : public Gtk::Window
{
  public:
    Application() : button_("TypeTrace")
    {
        button_.signal_clicked().connect(sigc::mem_fun(*this, &Application::on_button_clicked));
        set_child(button_);
    }

  private:
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    auto on_button_clicked() -> void
    {
        std::println("TypeTrace Frontend Started!");
    }

    Gtk::Button button_;
};

} // namespace typetrace::frontend

#endif // TYPETRACE_FRONTEND_APPLICATION_HPP
