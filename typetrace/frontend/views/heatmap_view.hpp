#ifndef TYPETRACE_HEATMAP_VIEW_HPP
#define TYPETRACE_HEATMAP_VIEW_HPP

#include "models/keyboard_layouts.hpp"
#include "models/keystroke_store.hpp"
#include "utils/color_utils.hpp"

#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>
#include <gtkmm/stylecontext.h>
#include <map>
#include <memory>
#include <string>

namespace typetrace::frontend::views {

class HeatmapView : public Gtk::Box
{
  public:
    HeatmapView(Glib::RefPtr<Gio::Settings> settings, std::shared_ptr<KeystrokeStore> store)
        : m_settings(std::move(settings)),
          m_keystroke_store(std::move(store))
    {
        // Load UI from resource
        auto builder = Gtk::Builder::create_from_resource("/edu/ost/typetrace/ui/heatmap.ui");

        // Get the root widget from the builder.
        // We assume the .ui file describes a Box or similar container.
        // We append it to this HeatmapView (which is a Box).
        auto* root =
          builder->get_widget<Gtk::Widget>("heatmap"); // Try explicit ID "heatmap" or similar?
        if (!root) {
            // Blueprint usually uses the ID defined in the .blp file.
            // If ID is not found, maybe get first object.
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

        m_keyboard_container = builder->get_widget<Gtk::Box>("keyboard_container");
        m_zoom_in_button = builder->get_widget<Gtk::Button>("zoom_in_button");
        m_zoom_out_button = builder->get_widget<Gtk::Button>("zoom_out_button");

        if (m_zoom_in_button) {
            m_zoom_in_button->signal_clicked().connect([this]() { on_zoom_clicked(5); });
        }
        if (m_zoom_out_button) {
            m_zoom_out_button->signal_clicked().connect([this]() { on_zoom_clicked(-5); });
        }

        m_settings->signal_changed().connect([this](const Glib::ustring& key) {
            if (key == "keyboard-layout") {
                on_keyboard_layout_changed(key);
            } else if (key.find("heatmap-") == 0 || key.find("use-") == 0) {
                update_colors();
            }
        });

        m_css_provider = Gtk::CssProvider::create();
        Gtk::StyleContext::add_provider_for_display(
          Gdk::Display::get_default(), m_css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        m_layout = m_settings->get_string("keyboard-layout");
        build_keyboard();

        m_keystroke_store->signal_changed().connect([this]() { update_colors(); });

        update_colors();
    }

    auto update_colors(const std::vector<KeystrokeEvent>* keystrokes_ptr = nullptr) -> void
    {
        std::vector<KeystrokeEvent> all_keystrokes;
        if (!keystrokes_ptr) {
            all_keystrokes = m_keystroke_store->get_all_keystrokes();
        }
        const auto& keystrokes = keystrokes_ptr ? *keystrokes_ptr : all_keystrokes;

        int most_pressed = m_keystroke_store->get_highest_count();
        if (most_pressed == 0) {
            most_pressed = 1;
        }

        auto color_scheme = utils::get_color_scheme(m_settings);
        std::string gradient_css = color_scheme->get_gradient_css();

        std::string css_data = gradient_css + "\n";

        // Reset tooltips
        for (auto& [scancode, label] : m_key_widgets) {
            label->set_tooltip_text("");
        }

        for (const auto& k : keystrokes) {
            if (m_key_widgets.count(k.key_code)) {
                auto label = m_key_widgets[k.key_code];
                std::string css_class = std::format("scancode-{}", k.key_code);
                float normalized = static_cast<float>(k.count) / most_pressed;

                auto [bg_color, text_color] = color_scheme->calculate_color_for_key(normalized);

                std::string rule = std::format(
                  ".{} {{ background-color: {}; color: {}; }}", css_class, bg_color, text_color);

                css_data += rule + "\n";

                label->set_css_classes({css_class});
                label->set_tooltip_text(std::to_string(k.count));
            }
        }

        m_css_provider->load_from_data(css_data);
    }

  private:
    auto on_zoom_clicked(int amount) -> void
    {
        int size = std::max(40, m_settings->get_int("key-size") + amount);
        m_settings->set_int("key-size", size);
        for (auto& [code, label] : m_key_widgets) {
            label->set_size_request(size, size);
        }
    }

    auto on_keyboard_layout_changed(const Glib::ustring& key) -> void
    {
        m_layout = m_settings->get_string("keyboard-layout");
        m_key_widgets.clear();
        build_keyboard();
        update_colors();
    }

    auto build_keyboard() -> void
    {
        if (m_keyboard_container) {
            // Remove all children
            while (auto child = m_keyboard_container->get_first_child()) {
                m_keyboard_container->remove(*child);
            }

            // Select layout
            const std::vector<std::vector<std::tuple<int, std::string, bool>>>* layout_ptr =
              &US_QWERTY;
            if (m_layout == "de_CH") {
                layout_ptr = &DE_CH;
            }

            for (const auto& row : *layout_ptr) {
                auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
                row_box->set_spacing(5);
                m_keyboard_container->append(*row_box);

                for (const auto& [scancode, label_text, is_expanded] : row) {
                    auto label = Gtk::make_managed<Gtk::Label>(label_text);
                    if (is_expanded) {
                        label->set_hexpand(true);
                    }
                    int size = m_settings->get_int("key-size");
                    label->set_size_request(size, size);

                    m_key_widgets[scancode] = label;
                    row_box->append(*label);
                }
            }
        }
    }

    Glib::RefPtr<Gio::Settings> m_settings;
    std::shared_ptr<KeystrokeStore> m_keystroke_store;
    Glib::RefPtr<Gtk::CssProvider> m_css_provider;

    Gtk::Box* m_keyboard_container = nullptr;
    Gtk::Button* m_zoom_in_button = nullptr;
    Gtk::Button* m_zoom_out_button = nullptr;

    std::map<int, Gtk::Label*> m_key_widgets;
    std::string m_layout;
};

} // namespace typetrace::frontend::views

#endif // TYPETRACE_HEATMAP_VIEW_HPP
