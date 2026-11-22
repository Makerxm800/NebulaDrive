#include "database.h"
#include "sqlite3.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <cstring>
#include <iomanip>
#include <cstdio>

namespace fs = std::filesystem;

// Escapes the LIKE wildcard characters (%, _) and the escape character itself,
// so user input is matched literally instead of as a pattern.
static std::string likeEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        if (c == '\\' || c == '%' || c == '_') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

// --- Database Implementation ---

Database::Database() : db(nullptr), dbPath("") {}

Database::~Database() {
    close();
}

Database& Database::instance() {
    static Database instance;
    return instance;
}

bool Database::initialize(const std::string& path) {
    dbPath = path;

    // Ensure parent directory exists
    fs::path dbFilePath(path);
    fs::path parentDir = dbFilePath.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir)) {
        try {
            fs::create_directories(parentDir);
        } catch (const std::exception& e) {
            std::cerr << "[DB] Failed to create directory: " << e.what() << std::endl;
            return false;
        }
    }

    int result = sqlite3_open_v2(path.c_str(), &db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (result != SQLITE_OK) {
        std::cerr << "[DB] Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Enable WAL mode for better performance
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    return createTables();
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        dbPath.clear();
    }
}

bool Database::is_open() const {
    return db != nullptr;
}

bool Database::createTables() {
    const char* sql =
        // Files table
        "CREATE TABLE IF NOT EXISTS files ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL,"
        "    path TEXT NOT NULL UNIQUE,"
        "    parent_folder TEXT DEFAULT '',"
        "    mime_type TEXT DEFAULT 'application/octet-stream',"
        "    size INTEGER DEFAULT 0,"
        "    checksum TEXT DEFAULT '',"
        "    created_at TEXT NOT NULL,"
        "    modified_at TEXT NOT NULL,"
        "    tags TEXT DEFAULT '',"
        "    is_favorite INTEGER DEFAULT 0,"
        "    is_deleted INTEGER DEFAULT 0,"
        "    deleted_at TEXT DEFAULT ''"
        ");"

        // Folders table
        "CREATE TABLE IF NOT EXISTS folders ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL,"
        "    path TEXT NOT NULL UNIQUE,"
        "    parent_path TEXT DEFAULT '',"
        "    created_at TEXT NOT NULL"
        ");"

        // Backups table
        "CREATE TABLE IF NOT EXISTS backups ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    name TEXT NOT NULL,"
        "    path TEXT NOT NULL,"
        "    size INTEGER DEFAULT 0,"
        "    file_count INTEGER DEFAULT 0,"
        "    created_at TEXT NOT NULL,"
        "    status TEXT DEFAULT 'completed',"
        "    description TEXT DEFAULT ''"
        ");"

        // History table
        "CREATE TABLE IF NOT EXISTS history ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    action TEXT NOT NULL,"
        "    file_path TEXT DEFAULT '',"
        "    details TEXT DEFAULT '',"
        "    timestamp TEXT NOT NULL"
        ");"

        // Settings table
        "CREATE TABLE IF NOT EXISTS settings ("
        "    key TEXT PRIMARY KEY,"
        "    value TEXT NOT NULL"
        ");"

        // Create indexes for faster queries
        "CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);"
        "CREATE INDEX IF NOT EXISTS idx_files_name ON files(name);"
        "CREATE INDEX IF NOT EXISTS idx_files_deleted ON files(is_deleted);"
        "CREATE INDEX IF NOT EXISTS idx_files_favorite ON files(is_favorite);"
        "CREATE INDEX IF NOT EXISTS idx_files_checksum ON files(checksum);"
        "CREATE INDEX IF NOT EXISTS idx_history_timestamp ON history(timestamp);";

    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::cerr << "[DB] Failed to create tables: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    // Insert default settings
    sqlite3_exec(db, "INSERT OR IGNORE INTO settings (key, value) VALUES ('version', '1.1.0');", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "INSERT OR IGNORE INTO settings (key, value) VALUES ('theme', 'dark');", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "INSERT OR IGNORE INTO settings (key, value) VALUES ('language', 'en');", nullptr, nullptr, nullptr);

    return true;
}

std::string Database::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// --- File Operations ---

int Database::addFile(const std::string& name, const std::string& path, const std::string& parentFolder,
                      const std::string& mimeType, uint64_t size, const std::string& checksum) {
    std::string timestamp = getCurrentTimestamp();

    std::ostringstream sql;
    sql << "INSERT INTO files (name, path, parent_folder, mime_type, size, checksum, created_at, modified_at) "
        << "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, parentFolder.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, mimeType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 5, size);
    sqlite3_bind_text(stmt, 6, checksum.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, timestamp.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int id = static_cast<int>(sqlite3_last_insert_rowid(db));

    // Add history entry
    addHistory("upload", path, "File uploaded: " + name);

    return result == SQLITE_DONE ? id : -1;
}

bool Database::updateFile(int id, const std::string& name, const std::string& path, uint64_t size,
                          const std::string& parentFolder) {
    std::string timestamp = getCurrentTimestamp();

    // parent_folder is always bound so moving a file into the root folder
    // (parentFolder == "") is recorded too — the old conditional left the
    // stale parent path in place and the file kept showing up in its old folder.
    std::ostringstream sql;
    sql << "UPDATE files SET name = ?, path = ?, size = ?, modified_at = ?, parent_folder = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, size);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, parentFolder.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, id);

    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (result) {
        addHistory("update", path, "File updated: " + name);
    }

    return result;
}

std::optional<FileRecord> Database::getFile(int id) {
    const char* sql = "SELECT * FROM files WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.size = sqlite3_column_int64(stmt, 5);
        record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
        record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
        record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));

        sqlite3_finalize(stmt);
        return record;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<FileRecord> Database::getFileByPath(const std::string& path) {
    const char* sql = "SELECT * FROM files WHERE path = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.size = sqlite3_column_int64(stmt, 5);
        record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
        record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
        record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));

        sqlite3_finalize(stmt);
        return record;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<FileRecord> Database::listFiles(bool includeDeleted) {
    std::vector<FileRecord> files;

    std::ostringstream sql;
    sql << "SELECT * FROM files WHERE 1=1";
    if (!includeDeleted) {
        sql << " AND is_deleted = 0";
    }
    sql << " ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.size = sqlite3_column_int64(stmt, 5);
        record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
        record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
        record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));

        files.push_back(record);
    }

    sqlite3_finalize(stmt);
    return files;
}

// Reads the 13 columns of the files table into a record.
static FileRecord readFileRow(sqlite3_stmt* stmt) {
    FileRecord record;
    record.id = sqlite3_column_int(stmt, 0);
    record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    record.size = sqlite3_column_int64(stmt, 5);
    record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
    record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
    record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
    record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
    return record;
}

std::vector<FileRecord> Database::listFilesByFolder(const std::string& parentFolder,
                                                    bool includeDeleted) {
    std::vector<FileRecord> files;

    std::ostringstream sql;
    sql << "SELECT * FROM files WHERE parent_folder = ?";
    if (!includeDeleted) {
        sql << " AND is_deleted = 0";
    }
    sql << " ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }

    sqlite3_bind_text(stmt, 1, parentFolder.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        files.push_back(readFileRow(stmt));
    }

    sqlite3_finalize(stmt);
    return files;
}

std::vector<FileRecord> Database::listFavoriteFiles() {
    std::vector<FileRecord> files;

    const char* sql = "SELECT * FROM files WHERE is_favorite = 1 AND is_deleted = 0 ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        files.push_back(readFileRow(stmt));
    }

    sqlite3_finalize(stmt);
    return files;
}

std::vector<FileRecord> Database::searchFiles(const std::string& pattern) {
    std::vector<FileRecord> files;

    std::string searchPattern = "%" + likeEscape(pattern) + "%";

    std::ostringstream sql;
    sql << "SELECT * FROM files WHERE name LIKE ? ESCAPE '\\' AND is_deleted = 0 ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }

    sqlite3_bind_text(stmt, 1, searchPattern.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.size = sqlite3_column_int64(stmt, 5);
        record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
        record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
        record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));

        files.push_back(record);
    }

    sqlite3_finalize(stmt);
    return files;
}

std::vector<FileRecord> Database::searchByTag(const std::string& tag) {
    std::vector<FileRecord> files;

    std::string searchTag = "%" + likeEscape(tag) + "%";

    std::ostringstream sql;
    sql << "SELECT * FROM files WHERE tags LIKE ? ESCAPE '\\' AND is_deleted = 0 ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return files;
    }

    sqlite3_bind_text(stmt, 1, searchTag.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.size = sqlite3_column_int64(stmt, 5);
        record.checksum = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        record.modifiedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        record.tags = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        record.isFavorite = sqlite3_column_int(stmt, 10) != 0;
        record.isDeleted = sqlite3_column_int(stmt, 11) != 0;
        record.deletedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));

        files.push_back(record);
    }

    sqlite3_finalize(stmt);
    return files;
}

bool Database::toggleFavorite(int id) {
    const char* sql = "UPDATE files SET is_favorite = NOT is_favorite WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (result) {
        auto file = getFile(id);
        if (file) {
            addHistory("favorite", file->path, "Toggled favorite for: " + file->name);
        }
    }

    return result;
}

bool Database::softDelete(int id) {
    std::string timestamp = getCurrentTimestamp();

    std::ostringstream sql;
    sql << "UPDATE files SET is_deleted = 1, deleted_at = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (result) {
        auto file = getFile(id);
        if (file) {
            addHistory("delete", file->path, "Moved to trash: " + file->name);
        }
    }

    return result;
}

bool Database::hardDelete(int id) {
    const char* sql = "DELETE FROM files WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (result) {
        addHistory("permanent_delete", "", "Permanently deleted file ID: " + std::to_string(id));
    }

    return result;
}

bool Database::restoreFile(int id) {
    const char* sql = "UPDATE files SET is_deleted = 0, deleted_at = '' WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    if (result) {
        auto file = getFile(id);
        if (file) {
            addHistory("restore", file->path, "Restored from trash: " + file->name);
        }
    }

    return result;
}

int Database::emptyTrash() {
    const char* sql = "DELETE FROM files WHERE is_deleted = 1;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_step(stmt);
    int count = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (count > 0) {
        addHistory("empty_trash", "", "Emptied trash: " + std::to_string(count) + " files");
    }

    return count;
}

// --- Folder Operations ---

int Database::createFolder(const std::string& name, const std::string& parentPath) {
    std::string timestamp = getCurrentTimestamp();
    std::string path = parentPath.empty() ? name : parentPath + "/" + name;

    std::ostringstream sql;
    sql << "INSERT INTO folders (name, path, parent_path, created_at) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, parentPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int id = static_cast<int>(sqlite3_last_insert_rowid(db));

    addHistory("create_folder", path, "Created folder: " + name);

    return result == SQLITE_DONE ? id : -1;
}

std::vector<FileRecord> Database::listFolders() {
    std::vector<FileRecord> folders;

    const char* sql = "SELECT * FROM folders ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return folders;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.isFavorite = false;
        record.isDeleted = false;
        record.size = 0;

        folders.push_back(record);
    }

    sqlite3_finalize(stmt);
    return folders;
}

// Reads the 5 columns of the folders table into a record.
static FileRecord readFolderRow(sqlite3_stmt* stmt) {
    FileRecord record;
    record.id = sqlite3_column_int(stmt, 0);
    record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    record.parentFolder = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    record.isFavorite = false;
    record.isDeleted = false;
    record.size = 0;
    return record;
}

std::vector<FileRecord> Database::listFoldersByParent(const std::string& parentPath) {
    std::vector<FileRecord> folders;

    const char* sql = "SELECT * FROM folders WHERE parent_path = ? ORDER BY name ASC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return folders;
    }

    sqlite3_bind_text(stmt, 1, parentPath.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        folders.push_back(readFolderRow(stmt));
    }

    sqlite3_finalize(stmt);
    return folders;
}

std::optional<FileRecord> Database::getFolderByPath(const std::string& path) {
    const char* sql = "SELECT * FROM folders WHERE path = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record = readFolderRow(stmt);
        sqlite3_finalize(stmt);
        return record;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::optional<FileRecord> Database::getFolderById(int id) {
    const char* sql = "SELECT * FROM folders WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord record = readFolderRow(stmt);
        sqlite3_finalize(stmt);
        return record;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool Database::deleteFolder(int id) {
    const char* sql = "DELETE FROM folders WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool Database::updateFolderParent(int id, const std::string& parentPath) {
    const char* sql = "UPDATE folders SET parent_path = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, parentPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool Database::renameFolderPath(int id, const std::string& path, const std::string& parentPath) {
    const char* sql = "UPDATE folders SET path = ?, parent_path = ?, name = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // The display name is the last segment of the path.
    std::string name = path;
    size_t slash = name.find_last_of('/');
    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parentPath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool Database::renameFilePath(int id, const std::string& path, const std::string& parentFolder) {
    const char* sql = "UPDATE files SET path = ?, parent_folder = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parentFolder.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::string Database::getSetting(const std::string& key) {
    const char* sql = "SELECT value FROM settings WHERE key = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return "";
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    std::string value;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) value = reinterpret_cast<const char*>(text);
    }

    sqlite3_finalize(stmt);
    return value;
}

bool Database::setSetting(const std::string& key, const std::string& value) {
    const char* sql = "INSERT INTO settings (key, value) VALUES (?, ?) "
                      "ON CONFLICT(key) DO UPDATE SET value = excluded.value;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_STATIC);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

// --- Backup Operations ---

int Database::addBackup(const std::string& name, const std::string& path, uint64_t size, int fileCount) {
    std::string timestamp = getCurrentTimestamp();

    std::ostringstream sql;
    sql << "INSERT INTO backups (name, path, size, file_count, created_at, status) "
        << "VALUES (?, ?, ?, ?, ?, 'completed');";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, size);
    sqlite3_bind_int(stmt, 4, fileCount);
    sqlite3_bind_text(stmt, 5, timestamp.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    int id = static_cast<int>(sqlite3_last_insert_rowid(db));

    addHistory("backup", path, "Backup created: " + name);

    return result == SQLITE_DONE ? id : -1;
}

std::vector<BackupRecord> Database::listBackups() {
    std::vector<BackupRecord> backups;

    const char* sql = "SELECT * FROM backups ORDER BY created_at DESC;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return backups;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BackupRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.size = sqlite3_column_int64(stmt, 3);
        record.fileCount = sqlite3_column_int(stmt, 4);
        record.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        record.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        record.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));

        backups.push_back(record);
    }

    sqlite3_finalize(stmt);
    return backups;
}

bool Database::deleteBackup(int id) {
    const char* sql = "DELETE FROM backups WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return result;
}

// --- History Operations ---

int Database::addHistory(const std::string& action, const std::string& filePath, const std::string& details) {
    std::string timestamp = getCurrentTimestamp();

    std::ostringstream sql;
    sql << "INSERT INTO history (action, file_path, details, timestamp) VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, filePath.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, details.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, timestamp.c_str(), -1, SQLITE_STATIC);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return result == SQLITE_DONE ? static_cast<int>(sqlite3_last_insert_rowid(db)) : -1;
}

std::vector<HistoryRecord> Database::getHistory(int limit) {
    std::vector<HistoryRecord> history;

    std::ostringstream sql;
    sql << "SELECT * FROM history ORDER BY timestamp DESC LIMIT ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return history;
    }

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HistoryRecord record;
        record.id = sqlite3_column_int(stmt, 0);
        record.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.filePath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        history.push_back(record);
    }

    sqlite3_finalize(stmt);
    return history;
}

void Database::clearHistory() {
    sqlite3_exec(db, "DELETE FROM history;", nullptr, nullptr, nullptr);
}

// --- Statistics ---

StorageStats Database::getStats() {
    StorageStats stats = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    // Total files (not deleted)
    {
        const char* sql = "SELECT COUNT(*), COALESCE(SUM(size), 0) FROM files WHERE is_deleted = 0;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.totalFiles = sqlite3_column_int64(stmt, 0);
                stats.totalSize = sqlite3_column_int64(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Total folders
    {
        const char* sql = "SELECT COUNT(*) FROM folders;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.totalFolders = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Trash files
    {
        const char* sql = "SELECT COUNT(*), COALESCE(SUM(size), 0) FROM files WHERE is_deleted = 1;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.trashFiles = sqlite3_column_int64(stmt, 0);
                stats.trashSize = sqlite3_column_int64(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Favorite files
    {
        const char* sql = "SELECT COUNT(*) FROM files WHERE is_favorite = 1 AND is_deleted = 0;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.favoriteFiles = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Backup count
    {
        const char* sql = "SELECT COUNT(*) FROM backups;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.backupCount = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    // Media counts by type.
    {
        const char* sql = "SELECT mime_type FROM files WHERE is_deleted = 0;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* type = sqlite3_column_text(stmt, 0);
                if (!type) continue;
                std::string mime = reinterpret_cast<const char*>(type);
                if (mime.rfind("video/", 0) == 0) ++stats.videoFiles;
                else if (mime.rfind("image/", 0) == 0) ++stats.imageFiles;
                else if (mime.rfind("audio/", 0) == 0) ++stats.audioFiles;
            }
            sqlite3_finalize(stmt);
        }
    }

    return stats;
}

// --- Utility ---

std::string Database::getDbPath() const {
    return dbPath;
}

bool Database::vacuum() {
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, "VACUUM;", nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK) {
        std::cerr << "[DB] Vacuum failed: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

std::string Database::exportSql() {
    std::string result;
    char** pResult;
    int nRow, nColumn;
    char* errMsg;

    // Export files table
    if (sqlite3_get_table(db, "SELECT * FROM files;", &pResult, &nRow, &nColumn, &errMsg) == SQLITE_OK) {
        result += "-- Files\n";
        for (int i = 0; i < (nRow + 1) * nColumn; i++) {
            result += pResult[i];
            if ((i + 1) % nColumn == 0) {
                result += "\n";
            } else {
                result += "\t";
            }
        }
        sqlite3_free_table(pResult);
    }

    sqlite3_free(errMsg);
    return result;
}
