#ifndef TYPETRACE_VERBOSE_VIEW_HPP
#define TYPETRACE_VERBOSE_VIEW_HPP

#include "models/keystroke_object.hpp"
#include "models/keystroke_store.hpp"

#include <giomm/liststore.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/columnview.h>
#include <gtkmm/columnviewcolumn.h>
#include <gtkmm/label.h>
#include <gtkmm/listitem.h>
#include <gtkmm/noselection.h>
#include <gtkmm/numericsorter.h>
#include <gtkmm/signallistitemfactory.h>
#include <gtkmm/singleselection.h>
#include <gtkmm/sortlistmodel.h>
#include <gtkmm/stringsorter.h>
#include <memory>

namespace typetrace::frontend::views {

class VerboseView : public Gtk::Box
{
  public:
    explicit VerboseView(std::shared_ptr<KeystrokeStore> store)
        : m_keystroke_store(std::move(store))
    {
        auto builder = Gtk::Builder::create_from_resource("/edu/ost/typetrace/ui/verbose.ui");

        auto* root = builder->get_widget<Gtk::Widget>("verbose");
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

        m_column_view = builder->get_widget<Gtk::ColumnView>("column_view");

        m_list_store = Gio::ListStore<models::KeystrokeObject>::create();
        m_sort_model = Gtk::SortListModel::create(m_list_store);
        m_selection_model = Gtk::SingleSelection::create(m_sort_model);

        if (m_column_view) {
            m_column_view->set_model(m_selection_model);
            setup_columns();
            m_sort_model->set_sorter(m_column_view->get_sorter());
        }

        m_keystroke_store->signal_changed().connect([this]() { update(); });
        update();
    }

    auto update() -> void
    {
        auto keystrokes = m_keystroke_store->get_all_keystrokes();
        m_list_store->remove_all();
        for (const auto& k : keystrokes) {
            m_list_store->append(models::KeystrokeObject::create(k));
        }
    }

  private:
    auto setup_columns() -> void
    {
        // Count Column
        auto factory_count = Gtk::SignalListItemFactory::create();
        factory_count->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_halign(Gtk::Align::START);
            list_item->set_child(*label);
        });
        factory_count->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(list_item->get_item());
            auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
            if (row && label) {
                label->set_text(std::to_string(row->get_count()));
            }
        });

        auto col_count = Gtk::ColumnViewColumn::create("Count", factory_count);
        // Sorter for Count
        auto expression_count =
          Gtk::ClosureExpression<int>::create([](const Glib::RefPtr<Glib::Object>& obj) {
              auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(obj);
              return row ? row->get_count() : 0;
          });
        col_count->set_sorter(Gtk::NumericSorter::create(expression_count));
        m_column_view->append_column(col_count);

        // Key Name Column
        auto factory_name = Gtk::SignalListItemFactory::create();
        factory_name->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_halign(Gtk::Align::START);
            list_item->set_child(*label);
        });
        factory_name->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(list_item->get_item());
            auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
            if (row && label) {
                label->set_text(row->get_key_name());
            }
        });

        auto col_name = Gtk::ColumnViewColumn::create("Key Name", factory_name);
        auto expression_name =
          Gtk::ClosureExpression<Glib::ustring>::create([](const Glib::RefPtr<Glib::Object>& obj) {
              auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(obj);
              return row ? row->get_key_name() : Glib::ustring("");
          });
        col_name->set_sorter(Gtk::StringSorter::create(expression_name));
        m_column_view->append_column(col_name);

        // Scan Code Column
        auto factory_code = Gtk::SignalListItemFactory::create();
        factory_code->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto label = Gtk::make_managed<Gtk::Label>();
            label->set_halign(Gtk::Align::START);
            list_item->set_child(*label);
        });
        factory_code->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& list_item) {
            auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(list_item->get_item());
            auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
            if (row && label) {
                label->set_text(std::to_string(row->get_key_code()));
            }
        });

        auto col_code = Gtk::ColumnViewColumn::create("Scan Code", factory_code);
        auto expression_code =
          Gtk::ClosureExpression<int>::create([](const Glib::RefPtr<Glib::Object>& obj) {
              auto row = std::dynamic_pointer_cast<models::KeystrokeObject>(obj);
              return row ? row->get_key_code() : 0;
          });
        col_code->set_sorter(Gtk::NumericSorter::create(expression_code));
        m_column_view->append_column(col_code);

        // Initial Sort: Count Descending (needs code if API allows setting initial sort easily)
        // GTK4 ColumnView usually manages sort via clicking headers.
        // To set initial sort, we can use SortListModel directly or set sorter on the column.

        // In Python: self.column_view.sort_by_column(..., Descending)
        m_column_view->sort_by_column(col_count, Gtk::SortType::DESCENDING);
    }

    std::shared_ptr<KeystrokeStore> m_keystroke_store;

    Gtk::ColumnView* m_column_view = nullptr;
    Glib::RefPtr<Gio::ListStore<models::KeystrokeObject>> m_list_store;
    Glib::RefPtr<Gtk::SortListModel> m_sort_model;
    Glib::RefPtr<Gtk::SingleSelection> m_selection_model;
};

} // namespace typetrace::frontend::views

#endif // TYPETRACE_VERBOSE_VIEW_HPP
