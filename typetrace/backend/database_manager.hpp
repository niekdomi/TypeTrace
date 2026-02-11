#pragma once

#include "constants.hpp"
#include "errors.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "sql.hpp"
#include "types.hpp"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <vector>

namespace typetrace::backend {

class DatabaseManager final
{
  public:
    /// Factory method to create a DatabaseManager instance
    [[nodiscard]]
    static auto create(const std::filesystem::path& db_dir) -> std::expected<DatabaseManager, Error>
    {
        DatabaseManager manager;
        manager.db_file_ = db_dir / DB_FILE_NAME;

        auto& logger = common::Logger::instance();
        logger.info("Initializing database at: {}", manager.db_file_.string());

        try {
            if (!db_dir.empty() && !std::filesystem::exists(db_dir)) {
                logger.debug("Creating parent directories for database path: {}", db_dir.string());
                std::filesystem::create_directories(db_dir);
            }

            manager.db_ = std::make_unique<SQLite::Database>(manager.db_file_.string(),
                                                             static_cast<unsigned int>(SQLite::OPEN_READWRITE)
                                                               | static_cast<unsigned int>(SQLite::OPEN_CREATE));

            // WAL mode
            manager.db_->exec(common::OPTIMIZE_DATABASE_SQL);
            TRY(manager.create_tables());
            logger.info("Database tables created successfully");
        }
        catch (const SQLite::Exception& e) {
            return std::unexpected(make_database_error(
              std::format("Failed to open database '{}': {}", manager.db_file_.string(), e.what())));
        }
        catch (const std::filesystem::filesystem_error& e) {
            return std::unexpected(make_system_error(std::format("Filesystem error: {}", e.what())));
        }

        return manager;
    }

    /// Writes a buffer of keystroke events to the database
    [[nodiscard]]
    auto write_to_database(const std::vector<common::KeystrokeEvent>& buffer) -> std::expected<void, Error>
    {
        if (buffer.empty()) {
            return {};
        }

        try {
            SQLite::Transaction transaction(*db_);
            SQLite::Statement stmt(*db_, common::UPSERT_KEYSTROKE_SQL);

            for (const auto& event : buffer) {
                stmt.bind(1, static_cast<int>(event.key_code));
                stmt.bind(2, event.key_name.data());
                stmt.bind(3, event.date.data());

                stmt.exec();
                stmt.reset();
            }

            transaction.commit();

            common::Logger::instance().debug(
              "Inserted {} keystrokes into the database: {}", buffer.size(), db_file_.string());
        }
        catch (const SQLite::Exception& e) {
            return std::unexpected(make_database_error(std::format("Failed to write to database: {}", e.what())));
        }

        return {};
    }

  private:
    /// Private constructor - use create() factory method
    DatabaseManager() = default;

    /// Creates necessary database tables if they don't exist
    [[nodiscard]]
    auto create_tables() -> std::expected<void, Error>
    {
        try {
            db_->exec(common::CREATE_KEYSTROKES_TABLE_SQL);
        }
        catch (const SQLite::Exception& e) {
            return std::unexpected(make_database_error(std::format("Failed to create tables: {}", e.what())));
        }
        return {};
    }

    std::filesystem::path db_file_;
    std::unique_ptr<SQLite::Database> db_;
};

} // namespace typetrace::backend
