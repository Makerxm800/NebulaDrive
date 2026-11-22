#ifndef NEBULADRIVE_STORAGE_HANDLER_H
#define NEBULADRIVE_STORAGE_HANDLER_H

#include "database.h"
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// Converts a UTF-8 string into a filesystem path. On Windows the standard
// library treats narrow strings as the active ANSI code page, which would
// mangle any file name outside ASCII.
fs::path toPath(const std::string& utf8);

class StorageHandler {
public:
    StorageHandler(const std::string& storagePath, Database& db);

    // Initialize storage directories
    bool initialize();

    // --- File Operations ---

    // Import a file from the filesystem into storage
    bool importFile(const std::string& sourcePath, const std::string& destinationFolder = "");

    // Import multiple files
    int importFiles(const std::vector<std::string>& sourcePaths, const std::string& destinationFolder = "");

    // Copy a file within storage
    bool copyFile(int fileId, const std::string& destinationFolder);

    // Move a file within storage
    bool moveFile(int fileId, const std::string& destinationFolder);

    // Rename a file
    bool renameFile(int fileId, const std::string& newName);

    // Delete a file (soft delete)
    bool deleteFile(int fileId);

    // Permanently delete a file
    bool permanentlyDeleteFile(int fileId);

    // Restore a deleted file
    bool restoreFile(int fileId);

    // Export a file to the filesystem
    bool exportFile(int fileId, const std::string& destinationPath);

    // --- Folder Operations ---

    // Create a folder
    bool createFolder(const std::string& name, const std::string& parentPath = "");

    // Rename a folder (moves it on disk and updates the database)
    bool renameFolder(int folderId, const std::string& newName);

    // Delete a folder and everything inside it
    bool deleteFolder(int folderId);

    // List files in a folder
    std::vector<FileRecord> listFolder(const std::string& folderPath);

    // --- Search ---

    // Search files by name
    std::vector<FileRecord> search(const std::string& query);

    // --- Trash ---

    // Permanently delete everything in the trash: the bytes on disk and the
    // database records. Returns the number of files deleted.
    int emptyTrash();

    // --- Backup ---

    // Create a backup of all files
    bool createBackup(const std::string& backupName = "");

    // List backups
    std::vector<BackupRecord> listBackups();

    // Delete a backup
    bool deleteBackup(int backupId);

    // --- Storage Info ---

    // Get storage statistics
    StorageStats getStats();

    // Get storage path
    std::string getStoragePath() const;

    // Get files directory
    std::string getFilesPath() const;

    // --- MIME Type Detection ---

    static std::string detectMimeType(const std::string& filePath);

    // --- Utility ---

    // Map a live storage path to its counterpart inside the trash folder
    std::string trashPathFor(const FileRecord& file);

    // Format file size
    static std::string formatSize(uint64_t bytes);

    // Validate file name
    static bool isValidFileName(const std::string& name);


private:
    std::string storagePath;
    std::string filesPath;
    std::string trashPath;
    std::string backupPath;
    Database& database;

    // Generate unique path
    std::string generateUniquePath(const std::string& name, const std::string& folder);

    // Compute file checksum
    std::string computeFileChecksum(const std::string& filePath);

    // Get file extension
    static std::string getExtension(const std::string& filePath);

};

#endif // NEBULADRIVE_STORAGE_HANDLER_H
