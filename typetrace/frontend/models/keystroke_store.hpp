#ifndef TYPETRACE_KEYSTROKE_STORE_HPP
#define TYPETRACE_KEYSTROKE_STORE_HPP

#include "types.hpp"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <glibmm/object.h>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace typetrace::frontend {

class KeystrokeStore : public Glib::Object
{
  public:
    explicit KeystrokeStore(const std::string& db_path) : Glib::ObjectBase(typeid(KeystrokeStore))
    {
        try {
            m_db = std::make_unique<SQLite::Database>(db_path, SQLite::OPEN_READONLY);
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to open database: " << e.what() << '\n';
            throw;
        }
    }

    ~KeystrokeStore() override = default;

    [[nodiscard]]
    auto get_all_keystrokes() -> std::vector<KeystrokeEvent>
    {
        std::vector<KeystrokeEvent> keystrokes;
        try {
            SQLite::Statement query(
              *m_db,
              "SELECT key_code, key_name, SUM(count) as total_count " "FROM keystrokes GROUP BY key_code, key_name ORDER BY total_count DESC");

            while (query.executeStep()) {
                KeystrokeEvent event{.key_code = static_cast<uint32_t>(query.getColumn(0).getInt()),
                                     .key_name = query.getColumn(1).getString(),
                                     .date = "",
                                     .count = query.getColumn(2).getInt()};
                keystrokes.push_back(event);
            }
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to query keystrokes: " << e.what() << '\n';
        }
        return keystrokes;
    }

    [[nodiscard]]
    auto get_top_keystrokes(int limit, const std::optional<std::string>& date = std::nullopt)
      -> std::vector<KeystrokeEvent>
    {
        std::vector<KeystrokeEvent> keystrokes;
        try {
            std::string sql =
              date.has_value()
                ? "SELECT key_code, key_name, count FROM keystrokes WHERE date = ? ORDER BY count " "DESC LIMIT ?"
                : "SELECT key_code, key_name, SUM(count) as total_count FROM keystrokes GROUP BY " "key_code, key_name ORDER BY total_count DESC LIMIT ?";

            SQLite::Statement query(*m_db, sql);
            if (date.has_value()) {
                query.bind(1, *date);
                query.bind(2, limit);
            } else {
                query.bind(1, limit);
            }

            while (query.executeStep()) {
                KeystrokeEvent event{.key_code = static_cast<uint32_t>(query.getColumn(0).getInt()),
                                     .key_name = query.getColumn(1).getString(),
                                     .date = date.value_or(""),
                                     .count = query.getColumn(2).getInt()};
                keystrokes.push_back(event);
            }
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to query top keystrokes: " << e.what() << '\n';
        }
        return keystrokes;
    }

    [[nodiscard]]
    auto get_daily_counts() -> std::vector<std::pair<std::string, int>>
    {
        std::vector<std::pair<std::string, int>> daily_counts;
        try {
            SQLite::Statement query(
              *m_db,
              "SELECT date, SUM(count) as total_count FROM keystrokes " "GROUP BY date ORDER BY date DESC LIMIT 7");

            while (query.executeStep()) {
                daily_counts.emplace_back(query.getColumn(0).getString(),
                                          query.getColumn(1).getInt());
            }
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to query daily counts: " << e.what() << '\n';
        }
        return daily_counts;
    }

    [[nodiscard]]
    auto get_total_count(const std::optional<std::string>& date = std::nullopt) -> int
    {
        try {
            std::string sql = date.has_value() ? "SELECT SUM(count) FROM keystrokes WHERE date = ?"
                                               : "SELECT SUM(count) FROM keystrokes";

            SQLite::Statement query(*m_db, sql);
            if (date.has_value()) {
                query.bind(1, *date);
            }

            if (query.executeStep()) {
                return query.getColumn(0).getInt();
            }
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to query total count: " << e.what() << '\n';
        }
        return 0;
    }

    [[nodiscard]]
    auto get_highest_count() -> int
    {
        try {
            SQLite::Statement query(
              *m_db,
              "SELECT MAX(total_count) FROM (" "  SELECT SUM(count) as total_count FROM keystrokes GROUP BY key_code" ")");

            if (query.executeStep()) {
                return query.getColumn(0).getInt();
            }
        }
        catch (const SQLite::Exception& e) {
            std::cerr << "Failed to query highest count: " << e.what() << '\n';
        }
        return 1;
    }

    auto signal_changed() -> sigc::signal<void()>
    {
        return m_signal_changed;
    }

    void refresh()
    {
        m_signal_changed.emit();
    }

  private:
    std::unique_ptr<SQLite::Database> m_db;
    sigc::signal<void()> m_signal_changed;
};

} // namespace typetrace::frontend

#endif // TYPETRACE_KEYSTROKE_STORE_HPP
