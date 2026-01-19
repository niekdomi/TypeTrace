#ifndef TYPETRACE_STATISTICS_VIEW_HPP
#define TYPETRACE_STATISTICS_VIEW_HPP

#include "models/keystroke_store.hpp"

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/calendar.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/spinbutton.h>
#include <memory>

namespace typetrace::frontend::views {

class StatisticsView : public Gtk::Box
{
  public:
    explicit StatisticsView(std::shared_ptr<KeystrokeStore> store)
        : m_keystroke_store(std::move(store))
    {
        auto builder = Gtk::Builder::create_from_resource("/edu/ost/typetrace/ui/statistics.ui");

        auto* root = builder->get_widget<Gtk::Widget>("statistics");
        if (!root) {
            auto objects = builder->get_objects();
            if (!objects.empty()) {
                root = dynamic_cast<Gtk::Widget*>(objects[0].get());
            }
        }
        if (root) {
            root->set_hexpand(true);
            root->set_vexpand(true);
            append(*root);
        }

        m_drawing_area = builder->get_widget<Gtk::DrawingArea>("drawing_area");
        m_line_drawing_area = builder->get_widget<Gtk::DrawingArea>("line_drawing_area");
        m_bar_count_spin = builder->get_widget<Gtk::SpinButton>("bar_count_spin");
        m_calendar = builder->get_widget<Gtk::Calendar>("calendar");
        m_date_button = builder->get_widget<Gtk::Button>("date_button");
        m_clear_date_button = builder->get_widget<Gtk::Button>("clear_date_button");

        if (m_bar_count_spin) {
            m_bar_count_spin->set_range(1, 10);
            m_bar_count_spin->set_value(5);
            // Connect signals (placeholder)
        }

        if (m_drawing_area) {
            m_drawing_area->set_draw_func(sigc::mem_fun(*this, &StatisticsView::on_draw_pie_chart));
        }

        if (m_line_drawing_area) {
            m_line_drawing_area->set_draw_func(
              sigc::mem_fun(*this, &StatisticsView::on_draw_line_chart));
        }
    }

  private:
    auto on_draw_pie_chart(const Glib::RefPtr<Cairo::Context>& cr, int width, int height) -> void
    {
        // Placeholder for Pie Chart drawing
        cr->set_source_rgb(0.9, 0.9, 0.9);
        cr->paint();
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(10, 20);
        cr->show_text("Pie Chart Placeholder");
    }

    auto on_draw_line_chart(const Glib::RefPtr<Cairo::Context>& cr, int width, int height) -> void
    {
        // Placeholder for Line Chart drawing
        cr->set_source_rgb(0.95, 0.95, 0.95);
        cr->paint();
        cr->set_source_rgb(0, 0, 0);
        cr->move_to(10, 20);
        cr->show_text("Line Chart Placeholder");
    }

    std::shared_ptr<KeystrokeStore> m_keystroke_store;

    Gtk::DrawingArea* m_drawing_area = nullptr;
    Gtk::DrawingArea* m_line_drawing_area = nullptr;
    Gtk::SpinButton* m_bar_count_spin = nullptr;
    Gtk::Calendar* m_calendar = nullptr;
    Gtk::Button* m_date_button = nullptr;
    Gtk::Button* m_clear_date_button = nullptr;
};

} // namespace typetrace::frontend::views

#endif // TYPETRACE_STATISTICS_VIEW_HPP
