#include "SqliteHistoryRepository.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>

namespace copyclickk {

namespace {

void execOrThrow(sqlite3* db, const char* sql) {
  char* err = nullptr;
  const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    const std::string message = err != nullptr ? err : "unknown sqlite error";
    sqlite3_free(err);
    throw std::runtime_error(message);
  }
}

ClipboardItem rowToItem(sqlite3_stmt* stmt) {
  ClipboardItem item;
  item.id = static_cast<std::int64_t>(sqlite3_column_int64(stmt, 0));
  item.timestampMs = static_cast<std::int64_t>(sqlite3_column_int64(stmt, 1));
  item.mimeType = static_cast<ClipboardMimeType>(sqlite3_column_int(stmt, 2));

  const auto* payload = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 3));
  const int payloadSize = sqlite3_column_bytes(stmt, 3);
  if (payload != nullptr && payloadSize > 0) {
    item.payload.assign(payload, payload + payloadSize);
  }

  const auto* appText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
  if (appText != nullptr) {
    item.sourceApp = appText;
  }

  item.pinned = sqlite3_column_int(stmt, 5) != 0;
  return item;
}

}  // namespace

SqliteHistoryRepository::SqliteHistoryRepository(std::string dbPath) {
  const int rc = sqlite3_open(dbPath.c_str(), &db_);
  if (rc != SQLITE_OK) {
    const std::string message = sqlite3_errmsg(db_);
    sqlite3_close(db_);
    db_ = nullptr;
    throw std::runtime_error("failed to open sqlite db: " + message);
  }

  std::error_code ec;
  std::filesystem::permissions(dbPath,
                               std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace,
                               ec);

  initializeSchema();
}

SqliteHistoryRepository::~SqliteHistoryRepository() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

ClipboardItem SqliteHistoryRepository::add(ClipboardItem item) {
  const std::string hash = item.contentHash();

  sqlite3_stmt* findStmt = nullptr;
  execOrThrow(db_, "BEGIN IMMEDIATE TRANSACTION;");

  const char* findSql =
      "SELECT id, timestamp_ms, mime_type, payload, source_app, pinned "
      "FROM history WHERE content_hash = ?1 AND mime_type = ?2 LIMIT 1;";
  if (sqlite3_prepare_v2(db_, findSql, -1, &findStmt, nullptr) != SQLITE_OK) {
    execOrThrow(db_, "ROLLBACK;");
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_text(findStmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(findStmt, 2, static_cast<int>(item.mimeType));

  const int findRc = sqlite3_step(findStmt);
  if (findRc == SQLITE_ROW) {
    ClipboardItem existing = rowToItem(findStmt);
    sqlite3_finalize(findStmt);

    sqlite3_stmt* updateStmt = nullptr;
    const char* updateSql = "UPDATE history SET timestamp_ms = ?1 WHERE id = ?2;";
    if (sqlite3_prepare_v2(db_, updateSql, -1, &updateStmt, nullptr) != SQLITE_OK) {
      execOrThrow(db_, "ROLLBACK;");
      throw std::runtime_error(sqlite3_errmsg(db_));
    }

    sqlite3_bind_int64(updateStmt, 1, item.timestampMs);
    sqlite3_bind_int64(updateStmt, 2, existing.id);
    if (sqlite3_step(updateStmt) != SQLITE_DONE) {
      sqlite3_finalize(updateStmt);
      execOrThrow(db_, "ROLLBACK;");
      throw std::runtime_error(sqlite3_errmsg(db_));
    }

    sqlite3_finalize(updateStmt);
    execOrThrow(db_, "COMMIT;");
    existing.timestampMs = item.timestampMs;
    return existing;
  }

  if (findRc != SQLITE_DONE) {
    sqlite3_finalize(findStmt);
    execOrThrow(db_, "ROLLBACK;");
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_finalize(findStmt);

  sqlite3_stmt* insertStmt = nullptr;
  const char* insertSql =
      "INSERT INTO history (timestamp_ms, mime_type, payload, source_app, pinned, content_hash) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6);";
  if (sqlite3_prepare_v2(db_, insertSql, -1, &insertStmt, nullptr) != SQLITE_OK) {
    execOrThrow(db_, "ROLLBACK;");
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(insertStmt, 1, item.timestampMs);
  sqlite3_bind_int(insertStmt, 2, static_cast<int>(item.mimeType));
  sqlite3_bind_blob(insertStmt, 3, item.payload.data(), static_cast<int>(item.payload.size()), SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt, 4, item.sourceApp.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(insertStmt, 5, item.pinned ? 1 : 0);
  sqlite3_bind_text(insertStmt, 6, hash.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insertStmt) != SQLITE_DONE) {
    sqlite3_finalize(insertStmt);
    execOrThrow(db_, "ROLLBACK;");
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_finalize(insertStmt);
  item.id = static_cast<std::int64_t>(sqlite3_last_insert_rowid(db_));
  execOrThrow(db_, "COMMIT;");
  return item;
}

std::vector<ClipboardItem> SqliteHistoryRepository::list(std::size_t limit) const {
  std::vector<ClipboardItem> items;
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "SELECT id, timestamp_ms, mime_type, payload, source_app, pinned "
      "FROM history ORDER BY pinned DESC, timestamp_ms DESC LIMIT ?1;";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(limit));

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    items.push_back(rowToItem(stmt));
  }

  sqlite3_finalize(stmt);
  return items;
}

std::optional<ClipboardItem> SqliteHistoryRepository::findById(std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "SELECT id, timestamp_ms, mime_type, payload, source_app, pinned "
      "FROM history WHERE id = ?1 LIMIT 1;";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(stmt, 1, id);
  const int rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    ClipboardItem item = rowToItem(stmt);
    sqlite3_finalize(stmt);
    return item;
  }

  sqlite3_finalize(stmt);
  return std::nullopt;
}

bool SqliteHistoryRepository::setPinned(std::int64_t id, bool pinned) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "UPDATE history SET pinned = ?1 WHERE id = ?2;";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int(stmt, 1, pinned ? 1 : 0);
  sqlite3_bind_int64(stmt, 2, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  const bool updated = sqlite3_changes(db_) > 0;
  sqlite3_finalize(stmt);
  return updated;
}

bool SqliteHistoryRepository::remove(std::int64_t id) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "DELETE FROM history WHERE id = ?1;";
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(stmt, 1, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  const bool removed = sqlite3_changes(db_) > 0;
  sqlite3_finalize(stmt);
  return removed;
}

void SqliteHistoryRepository::clear() {
  execOrThrow(db_, "DELETE FROM history;");
}

void SqliteHistoryRepository::clearUnpinned() {
  execOrThrow(db_, "DELETE FROM history WHERE pinned = 0;");
}

void SqliteHistoryRepository::trimToLimit(std::size_t limit) {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "DELETE FROM history WHERE id IN ("
      "SELECT id FROM history WHERE pinned = 0 ORDER BY timestamp_ms DESC LIMIT -1 OFFSET ("
      "SELECT CASE WHEN ?1 > COUNT(*) THEN ?1 - COUNT(*) ELSE 0 END FROM history WHERE pinned = 1"
      ")"
      ");";

  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(limit));
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw std::runtime_error(sqlite3_errmsg(db_));
  }

  sqlite3_finalize(stmt);
}

void SqliteHistoryRepository::initializeSchema() {
  execOrThrow(db_, "PRAGMA journal_mode=WAL;");
  execOrThrow(
      db_,
      "CREATE TABLE IF NOT EXISTS history ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "timestamp_ms INTEGER NOT NULL,"
      "mime_type INTEGER NOT NULL,"
      "payload BLOB NOT NULL,"
      "source_app TEXT NOT NULL DEFAULT '',"
      "pinned INTEGER NOT NULL DEFAULT 0,"
      "content_hash TEXT NOT NULL"
      ");");
  execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_history_order ON history(pinned DESC, timestamp_ms DESC);");
  execOrThrow(db_, "CREATE UNIQUE INDEX IF NOT EXISTS idx_history_dedupe ON history(mime_type, content_hash);");
}

}  // namespace copyclickk
