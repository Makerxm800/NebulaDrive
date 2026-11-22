#ifndef NEBULADRIVE_DATABASE_H
#define NEBULADRIVE_DATABASE_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <cstdint>

struct sqlite3;
struct sqlite3_stmt;

// File metadata stored in the database
struct FileRecord {
    int id;
    std::string name;
    std::string path;           // Relative path within storage
    std::string parentFolder;   // Parent folder path
    std::string mimeType;
    uint64_t size;
    std::string checksum;       // Content hash (FNV-1a)
    std::string createdAt;      // ISO 8601 timestamp
    std::string modifiedAt;
    std::string tags;           // Comma-separated tags
    bool isFavorite;
    bool isDeleted;             // Soft delete flag
    std::string deletedAt;
};

// Backup record
struct BackupRecord {
    int id;
    std::string name;
    std::string path;
    uint64_t size;
    int fileCount;
    std::string createdAt;
    std::string status;         // "completed", "failed", "in_progress"
    std::string description;
};

// Operation history entry
struct HistoryRecord {
    int id;
    std::string action;         // "upload", "delete", "rename", "move", "restore", "backup"
    std::string filePath;
    std::string details;
    std::string timestamp;
};

// Storage statistics
struct StorageStats {
    uint64_t totalFiles;
    uint64_t totalFolders;
    uint64_t totalSize;
    uint64_t trashFiles;
    uint64_t trashSize;
    uint64_t favoriteFiles;
    uint64_t backupCount;
    uint64_t videoFiles;   // files whose type is video/*
    uint64_t imageFiles;   // files whose type is image/*
    uint64_t audioFiles;   // files whose type is audio/*
};

class Database {
public:
    // Singleton-like interface
    static Database& instance();

    // Initialize database with the given path
    bool initialize(const std::string& dbPath);

    // Close database connection
    void close();

    // Check if database is open
    bool is_open() const;

    // --- File Operations ---

    // Add a new file record
    int addFile(const std::string& name, const std::string& path, const std::string& parentFolder,
                const std::string& mimeType, uint64_t size, const std::string& checksum);

    // Update file metadata
    bool updateFile(int id, const std::string& name, const std::string& path, uint64_t size,
                    const std::string& parentFolder = std::string());

    // Get file by ID
    std::optional<FileRecord> getFile(int id);

    // Get file by path
    std::optional<FileRecord> getFileByPath(const std::string& path);

    // List all files (with optional filter)
    std::vector<FileRecord> listFiles(bool includeDeleted = false);

    // List files that live directly inside the given folder.
    // Pass an empty string for the root folder.
    std::vector<FileRecord> listFilesByFolder(const std::string& parentFolder,
                                              bool includeDeleted = false);

    // List files flagged as favourites (not deleted).
    std::vector<FileRecord> listFavoriteFiles();

    // Search files by name pattern
    std::vector<FileRecord> searchFiles(const std::string& pattern);

    // Search files by tags
    std::vector<FileRecord> searchByTag(const std::string& tag);

    // Toggle favorite status
    bool toggleFavorite(int id);

    // Soft delete a file
    bool softDelete(int id);

    // Hard delete a file (permanent)
    bool hardDelete(int id);

    // Restore a deleted file
    bool restoreFile(int id);

    // Empty trash (hard delete all soft-deleted files)
    int emptyTrash();

    // --- Folder Operations ---

    // Create a folder record
    int createFolder(const std::string& name, const std::string& parentPath);

    // List all folders
    std::vector<FileRecord> listFolders();

    // List folders that live directly inside the given parent path.
    // Pass an empty string for the root folder.
    std::vector<FileRecord> listFoldersByParent(const std::string& parentPath);

    // Look up a single folder by its full path
    std::optional<FileRecord> getFolderByPath(const std::string& path);

    // Look up a single folder by its numeric id
    std::optional<FileRecord> getFolderById(int id);

    // Delete a folder record
    bool deleteFolder(int id);

    // Update a folder's parent path (used when moving folders)
    bool updateFolderParent(int id, const std::string& parentPath);

    // Rename a folder: update its path and parent path in one step
    bool renameFolderPath(int id, const std::string& path, const std::string& parentPath);

    // Update a file's path and parent folder without touching its name
    bool renameFilePath(int id, const std::string& path, const std::string& parentFolder);

    // --- Settings (key/value) ---

    // Read a setting value; returns empty string when the key is unknown
    std::string getSetting(const std::string& key);

    // Write a setting value
    bool setSetting(const std::string& key, const std::string& value);

    // --- Backup Operations ---

    // Record a new backup
    int addBackup(const std::string& name, const std::string& path, uint64_t size, int fileCount);

    // List all backups
    std::vector<BackupRecord> listBackups();

    // Delete backup record
    bool deleteBackup(int id);

    // --- History Operations ---

    // Add a history entry
    int addHistory(const std::string& action, const std::string& filePath, const std::string& details);

    // Get recent history
    std::vector<HistoryRecord> getHistory(int limit = 50);

    // Clear history
    void clearHistory();

    // --- Statistics ---

    // Get storage statistics
    StorageStats getStats();

    // --- Utility ---

    // Get database path
    std::string getDbPath() const;

    // Vacuum database (optimize)
    bool vacuum();

    // Export database to SQL text
    std::string exportSql();

private:
    Database();
    ~Database();

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTables();
    std::string getCurrentTimestamp();

    sqlite3* db;
    std::string dbPath;
};

// Convenience namespace for database access
namespace db {
    inline Database& get() { return Database::instance(); }
    inline bool init(const std::string& path) { return Database::instance().initialize(path); }
    inline void shutdown() { Database::instance().close(); }
}

#endif // NEBULADRIVE_DATABASE_H
