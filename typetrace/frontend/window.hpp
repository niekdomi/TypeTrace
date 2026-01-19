#ifndef TYPETRACE_WINDOW_HPP
#define TYPETRACE_WINDOW_HPP

#include "models/keystroke_store.hpp"
#include "views/heatmap_view.hpp"
#include "views/statistics_view.hpp"
#include "views/verbose_view.hpp"

#include <gio/gio.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/stack.h>
#include <iostream>
#include <memory>

namespace typetrace::frontend {

class Window : public Gtk::ApplicationWindow
{
  public:
    explicit Window(std::shared_ptr<KeystrokeStore> keystroke_store)
        : m_keystroke_store(std::move(keystroke_store))
    {
        set_title("TypeTrace");
        set_default_size(1'200, 800);

        // Load UI from Blueprint-compiled resource
        auto builder = Gtk::Builder::create_from_resource("/edu/ost/typetrace/ui/window.blp");

        // Get widgets from builder
        m_stack = builder->get_widget<Gtk::Stack>("stack");
        m_backend_toggle = builder->get_widget<Gtk::Button>("backend_toggle");

        // Initialize settings
        // Assuming schema id is "edu.ost.typetrace"
        m_settings = Gio::Settings::create("edu.ost.typetrace");

        if (m_stack != nullptr) {
            auto* heatmap_view =
              Gtk::make_managed<views::HeatmapView>(m_settings, m_keystroke_store);
            auto* statistics_view = Gtk::make_managed<views::StatisticsView>(m_keystroke_store);
            auto* verbose_view = Gtk::make_managed<views::VerboseView>(m_keystroke_store);

            heatmap_view->set_margin(20);
            statistics_view->set_margin(20);
            verbose_view->set_margin(20);

            m_stack->add(*heatmap_view, "heatmap", "Heatmap");
            m_stack->add(*statistics_view, "statistics", "Statistics");
            m_stack->add(*verbose_view, "verbose", "Verbose");
        }

        if (m_backend_toggle != nullptr) {
            m_backend_toggle->signal_clicked().connect(
              sigc::mem_fun(*this, &Window::on_backend_toggle_clicked));
        }

        // Set window content
        auto* content = builder->get_widget<Gtk::Widget>("toast_overlay");
        if (content != nullptr) {
            set_child(*content);
        }
    }

    ~Window() override = default;

  private:
    auto on_backend_toggle_clicked() -> void
    {
        if (m_is_backend_running) {
            std::cout << "Stopping backend...\n";
            // TODO: Implement backend stop via D-Bus
            m_backend_toggle->set_label("Backend stopped");
            m_backend_toggle->remove_css_class("backend-status-running");
            m_backend_toggle->add_css_class("backend-status-stopped");
            m_is_backend_running = false;
        } else {
            std::cout << "Starting backend...\n";
            // TODO: Implement backend start via D-Bus
            m_backend_toggle->set_label("Backend running");
            m_backend_toggle->remove_css_class("backend-status-stopped");
            m_backend_toggle->add_css_class("backend-status-running");
            m_is_backend_running = true;
        }
    }

    Gtk::Stack* m_stack{nullptr};
    Gtk::Button* m_backend_toggle{nullptr};
    bool m_is_backend_running{false};
    std::shared_ptr<KeystrokeStore> m_keystroke_store;
    Glib::RefPtr<Gio::Settings> m_settings;
};

} // namespace typetrace::frontend

#endif // TYPETRACE_WINDOW_HPP
