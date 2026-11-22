#include "storage_handler.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <map>
#include <cstring>

StorageHandler::StorageHandler(const std::string& storagePath, Database& db)
    : storagePath(storagePath), database(db) {
    filesPath = storagePath + "/files";
    trashPath = storagePath + "/trash";
    backupPath = storagePath + "/backups";
}

// Converts a UTF-8 string into a filesystem path. On Windows the standard
// library treats narrow strings as the active ANSI code page, so non-ASCII
// names would be mangled; wide paths keep them intact. On other platforms
// the narrow path is already UTF-8.
fs::path toPath(const std::string& utf8) {
    if (utf8.empty()) return fs::path();
#ifdef _WIN32
    int length = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (length <= 1) return fs::path(utf8);
    std::wstring wide(static_cast<size_t>(length - 1), 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], length);
    return fs::path(wide);
#else
    return fs::path(utf8);
#endif
}

bool StorageHandler::initialize() {
    try {
        // Create main directories
        std::vector<std::string> dirs = {filesPath, trashPath, backupPath};
        for (const auto& dir : dirs) {
            if (!fs::exists(toPath(dir))) {
                fs::create_directories(toPath(dir));
                std::cout << "[✓] Created directory: " << dir << std::endl;
            }
        }

        std::cout << "[✓] Storage handler initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to initialize storage: " << e.what() << std::endl;
        return false;
    }
}

// --- File Operations ---

bool StorageHandler::importFile(const std::string& sourcePath, const std::string& destinationFolder) {
    fs::path source = toPath(sourcePath);

    if (!fs::exists(source)) {
        std::cerr << "[✗] Source file not found: " << sourcePath << std::endl;
        return false;
    }

    if (!fs::is_regular_file(source)) {
        std::cerr << "[✗] Source is not a file: " << sourcePath << std::endl;
        return false;
    }

    // Generate unique path
    std::string fileName = source.filename().string();
    std::string relativePath = generateUniquePath(fileName, destinationFolder);
    std::string destFullPath = filesPath + "/" + relativePath;

    // Create destination directory if needed
    fs::path destDir = toPath(destFullPath).parent_path();
    if (!fs::exists(destDir)) {
        fs::create_directories(destDir);
    }

    // Copy file
    try {
        fs::copy_file(toPath(sourcePath), toPath(destFullPath), fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to copy file: " << e.what() << std::endl;
        return false;
    }

    // Get file info
    uint64_t size = fs::file_size(toPath(destFullPath));
    std::string mimeType = detectMimeType(destFullPath);
    std::string checksum = computeFileChecksum(destFullPath);

    // Add to database
    int id = database.addFile(fileName, relativePath, destinationFolder, mimeType, size, checksum);

    if (id > 0) {
        std::cout << "[✓] Imported: " << fileName << " (" << formatSize(size) << ")" << std::endl;
        return true;
    }

    return false;
}

int StorageHandler::importFiles(const std::vector<std::string>& sourcePaths, const std::string& destinationFolder) {
    int success = 0;
    int total = static_cast<int>(sourcePaths.size());

    std::cout << "[i] Importing " << total << " files..." << std::endl;

    for (size_t i = 0; i < sourcePaths.size(); ++i) {
        int percent = static_cast<int>((i + 1) * 100 / total);
        std::cout << "    [" << percent << "%] " << fs::path(sourcePaths[i]).filename().string() << "... ";

        if (importFile(sourcePaths[i], destinationFolder)) {
            success++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    }

    std::cout << "[✓] Imported " << success << "/" << total << " files" << std::endl;
    return success;
}

bool StorageHandler::copyFile(int fileId, const std::string& destinationFolder) {
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    std::string sourcePath = filesPath + "/" + file->path;
    if (!fs::exists(toPath(sourcePath))) {
        std::cerr << "[✗] Source file not found on disk" << std::endl;
        return false;
    }

    // Generate new path
    std::string newName = file->name;
    std::string relativePath = generateUniquePath(newName, destinationFolder);
    std::string destFullPath = filesPath + "/" + relativePath;

    // Create destination directory
    fs::path destDir = toPath(destFullPath).parent_path();
    if (!fs::exists(destDir)) {
        fs::create_directories(destDir);
    }

    // Copy file
    try {
        fs::copy_file(toPath(sourcePath), toPath(destFullPath), fs::copy_options::overwrite_existing);
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to copy: " << e.what() << std::endl;
        return false;
    }

    // Add to database
    int id = database.addFile(file->name, relativePath, destinationFolder, file->mimeType, file->size, file->checksum);

    if (id > 0) {
        std::cout << "[✓] Copied: " << file->name << std::endl;
        database.addHistory("copy", relativePath, "Copied from: " + file->path);
        return true;
    }

    return false;
}

bool StorageHandler::moveFile(int fileId, const std::string& destinationFolder) {
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    std::string sourcePath = filesPath + "/" + file->path;
    if (!fs::exists(toPath(sourcePath))) {
        std::cerr << "[✗] Source file not found on disk" << std::endl;
        return false;
    }

    // Generate new path
    std::string relativePath = generateUniquePath(file->name, destinationFolder);
    std::string destFullPath = filesPath + "/" + relativePath;

    // Create destination directory
    fs::path destDir = toPath(destFullPath).parent_path();
    if (!fs::exists(destDir)) {
        fs::create_directories(destDir);
    }

    // Move file
    try {
        fs::rename(toPath(sourcePath), toPath(destFullPath));
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to move: " << e.what() << std::endl;
        return false;
    }

    // Update database - the parent folder must change too, otherwise the file
    // keeps showing up in its old folder listing.
    bool result = database.updateFile(fileId, file->name, relativePath, file->size,
                                      destinationFolder);

    if (result) {
        std::cout << "[✓] Moved: " << file->name << " to "
                  << (destinationFolder.empty() ? "root" : destinationFolder) << std::endl;
        database.addHistory("move", relativePath, "Moved from: " + file->path);
        return true;
    }

    return false;
}

bool StorageHandler::renameFile(int fileId, const std::string& newName) {
    if (!isValidFileName(newName)) {
        std::cerr << "[✗] Invalid file name" << std::endl;
        return false;
    }

    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    std::string oldPath = filesPath + "/" + file->path;
    if (!fs::exists(toPath(oldPath))) {
        std::cerr << "[✗] File not found on disk" << std::endl;
        return false;
    }

    // Keep the original extension. If the caller already supplied one that
    // matches, do not append it a second time.
    fs::path oldFilePath = toPath(oldPath);
    std::string extension = oldFilePath.extension().string();
    std::string stem = newName;
    if (!extension.empty() && stem.size() >= extension.size() &&
        stem.compare(stem.size() - extension.size(), extension.size(), extension) == 0) {
        stem = stem.substr(0, stem.size() - extension.size());
    }
    std::string finalName = stem + extension;

    // New path, relative to the storage root.
    std::string newRelativePath = file->parentFolder.empty()
        ? finalName
        : file->parentFolder + "/" + finalName;
    std::string newFullPath = filesPath + "/" + newRelativePath;

    // Never clobber an existing file.
    if (fs::exists(toPath(newFullPath))) {
        std::cerr << "[✗] A file with that name already exists" << std::endl;
        return false;
    }

    // Rename on disk
    try {
        fs::rename(toPath(oldPath), toPath(newFullPath));
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to rename: " << e.what() << std::endl;
        return false;
    }

    // Update database
    bool result = database.updateFile(fileId, finalName, newRelativePath, file->size,
                                      file->parentFolder);

    if (result) {
        std::cout << "[✓] Renamed: " << file->name << " -> " << finalName << std::endl;
        database.addHistory("rename", newRelativePath, "Renamed from: " + file->name);
        return true;
    }

    return false;
}

bool StorageHandler::deleteFile(int fileId) {
    // Move the file into the trash folder on disk so the bytes are actually
    // recoverable, then flag the record as deleted.
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    std::string sourcePath = filesPath + "/" + file->path;
    if (fs::exists(toPath(sourcePath))) {
        std::string trashPath = trashPathFor(*file);
        try {
            fs::path trashDir = fs::path(trashPath).parent_path();
            if (!fs::exists(trashDir)) {
                fs::create_directories(trashDir);
            }
            fs::rename(toPath(sourcePath), toPath(trashPath));
        } catch (const std::exception& e) {
            // If the move fails the file stays where it is; the record is
            // still flagged so the UI stays consistent.
            std::cerr << "[!] Could not move file to trash: " << e.what() << std::endl;
        }
    }

    return database.softDelete(fileId);
}

bool StorageHandler::permanentlyDeleteFile(int fileId) {
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    // Delete from disk - the file may be in the trash folder or the live tree.
    for (const std::string& candidate : {filesPath + "/" + file->path, trashPathFor(*file)}) {
        if (fs::exists(toPath(candidate))) {
            try {
                fs::remove(toPath(candidate));
            } catch (const std::exception& e) {
                std::cerr << "[✗] Failed to delete file: " << e.what() << std::endl;
            }
        }
    }

    // Delete from database
    bool result = database.hardDelete(fileId);

    if (result) {
        std::cout << "[✓] Permanently deleted: " << file->name << std::endl;
        return true;
    }

    return false;
}

bool StorageHandler::restoreFile(int fileId) {
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    // Bring the bytes back from the trash folder.
    std::string trashFile = trashPathFor(*file);
    std::string liveFile = filesPath + "/" + file->path;
    if (fs::exists(toPath(trashFile))) {
        try {
            fs::path liveDir = fs::path(liveFile).parent_path();
            if (!fs::exists(liveDir)) {
                fs::create_directories(liveDir);
            }
            fs::rename(toPath(trashFile), toPath(liveFile));
        } catch (const std::exception& e) {
            std::cerr << "[!] Could not restore file from trash: " << e.what() << std::endl;
        }
    }

    bool result = database.restoreFile(fileId);

    if (result) {
        std::cout << "[✓] Restored: " << file->name << std::endl;
    }

    return result;
}

bool StorageHandler::exportFile(int fileId, const std::string& destinationPath) {
    auto file = database.getFile(fileId);
    if (!file) {
        std::cerr << "[✗] File not found: " << fileId << std::endl;
        return false;
    }

    std::string sourcePath = filesPath + "/" + file->path;
    if (!fs::exists(toPath(sourcePath))) {
        std::cerr << "[✗] File not found on disk" << std::endl;
        return false;
    }

    try {
        fs::path dest = toPath(destinationPath);
        fs::path destDir = dest.parent_path();
        if (!destDir.empty() && !fs::exists(destDir)) {
            fs::create_directories(destDir);
        }

        fs::copy_file(toPath(sourcePath), toPath(destinationPath), fs::copy_options::overwrite_existing);
        std::cout << "[✓] Exported: " << file->name << " to " << destinationPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to export: " << e.what() << std::endl;
        return false;
    }
}

// --- Folder Operations ---

bool StorageHandler::createFolder(const std::string& name, const std::string& parentPath) {
    if (!isValidFileName(name)) {
        std::cerr << "[✗] Invalid folder name" << std::endl;
        return false;
    }

    int id = database.createFolder(name, parentPath);

    if (id > 0) {
        // Mirror the folder on disk so files can actually be stored inside it.
        std::string folderDir = parentPath.empty() ? name : parentPath + "/" + name;
        try {
            fs::create_directories(toPath(filesPath + "/" + folderDir));
        } catch (const std::exception& e) {
            std::cerr << "[!] Could not create folder on disk: " << e.what() << std::endl;
        }
        std::cout << "[✓] Created folder: " << name << std::endl;
        return true;
    }

    return false;
}

bool StorageHandler::renameFolder(int folderId, const std::string& newName) {
    if (!isValidFileName(newName)) {
        std::cerr << "[✗] Invalid folder name" << std::endl;
        return false;
    }

    auto folder = database.getFolderById(folderId);
    if (!folder) {
        std::cerr << "[✗] Folder not found: " << folderId << std::endl;
        return false;
    }

    std::string oldPath = filesPath + "/" + folder->path;
    std::string parent = folder->parentFolder;
    std::string newPath = parent.empty() ? newName : parent + "/" + newName;
    std::string newFullPath = filesPath + "/" + newPath;

    if (fs::exists(toPath(newFullPath))) {
        std::cerr << "[✗] A folder with that name already exists" << std::endl;
        return false;
    }

    try {
        if (fs::exists(toPath(oldPath))) {
            fs::rename(toPath(oldPath), toPath(newFullPath));
        }
    } catch (const std::exception& e) {
        std::cerr << "[✗] Failed to rename folder: " << e.what() << std::endl;
        return false;
    }

    // Update the folder record and every descendant path.
    std::string oldPrefix = folder->path;
    std::string newPrefix = newPath;

    auto folders = database.listFolders();
    for (const auto& child : folders) {
        if (child.id == folderId) {
            continue;
        }
        // A descendant starts with the old path followed by a separator.
        if (child.path.size() > oldPrefix.size() &&
            child.path.compare(0, oldPrefix.size(), oldPrefix) == 0 &&
            child.path[oldPrefix.size()] == '/') {
            std::string suffix = child.path.substr(oldPrefix.size());
            std::string updated = newPrefix + suffix;
            database.renameFolderPath(child.id, updated, child.parentFolder == oldPrefix
                                                                  ? newPrefix
                                                                  : child.parentFolder);
        }
    }

    // Update the files inside as well.
    auto files = database.listFiles(true);
    for (const auto& file : files) {
        if (file.parentFolder.size() > oldPrefix.size() &&
            file.parentFolder.compare(0, oldPrefix.size(), oldPrefix) == 0 &&
            file.parentFolder[oldPrefix.size()] == '/') {
            std::string suffix = file.parentFolder.substr(oldPrefix.size());
            std::string updatedParent = newPrefix + suffix;
            std::string updatedPath = updatedParent + "/" + file.name;
            database.renameFilePath(file.id, updatedPath, updatedParent);
            // Move the bytes on disk.
            try {
                std::string from = filesPath + "/" + file.path;
                std::string to = filesPath + "/" + updatedPath;
                if (fs::exists(toPath(from))) {
                    fs::path toDir = fs::path(to).parent_path();
                    if (!fs::exists(toDir)) {
                        fs::create_directories(toDir);
                    }
                    fs::rename(toPath(from), toPath(to));
                }
            } catch (...) {
                // The record is already updated; a stray file on disk is not fatal.
            }
        }
    }

    database.renameFolderPath(folderId, newPath, parent);
    database.addHistory("rename", newPath, "Renamed folder from: " + oldPrefix);
    std::cout << "[✓] Renamed folder: " << folder->name << " -> " << newName << std::endl;
    return true;
}

bool StorageHandler::deleteFolder(int folderId) {
    auto folder = database.getFolderById(folderId);
    if (!folder) {
        std::cerr << "[✗] Folder not found: " << folderId << std::endl;
        return false;
    }

    std::string prefix = folder->path;

    // Remove every file inside the folder.
    auto files = database.listFiles(true);
    for (const auto& file : files) {
        bool inside = file.parentFolder == prefix ||
                      (file.parentFolder.size() > prefix.size() &&
                       file.parentFolder.compare(0, prefix.size(), prefix) == 0 &&
                       file.parentFolder[prefix.size()] == '/');
        if (!inside) continue;

        std::string full = filesPath + "/" + file.path;
        if (fs::exists(toPath(full))) {
            try {
                fs::remove(toPath(full));
            } catch (...) {
            }
        }
        database.hardDelete(file.id);
    }

    // Remove every sub-folder record.
    auto folders = database.listFolders();
    for (const auto& child : folders) {
        if (child.id == folderId) continue;
        if (child.path.size() > prefix.size() &&
            child.path.compare(0, prefix.size(), prefix) == 0 &&
            child.path[prefix.size()] == '/') {
            database.deleteFolder(child.id);
        }
    }

    // Remove the folder from disk.
    std::string folderDir = filesPath + "/" + prefix;
    if (fs::exists(toPath(folderDir))) {
        try {
            fs::remove_all(toPath(folderDir));
        } catch (...) {
        }
    }

    database.deleteFolder(folderId);
    database.addHistory("delete", prefix, "Deleted folder: " + folder->name);
    std::cout << "[✓] Deleted folder: " << folder->name << std::endl;
    return true;
}

std::vector<FileRecord> StorageHandler::listFolder(const std::string& folderPath) {
    return database.listFilesByFolder(folderPath, false);
}

// --- Search ---

std::vector<FileRecord> StorageHandler::search(const std::string& query) {
    return database.searchFiles(query);
}

// --- Trash ---

int StorageHandler::emptyTrash() {
    // Collect the trash records first, then remove the bytes on disk, then
    // delete the records — so a failure on disk never strands DB rows.
    auto trashed = database.listFiles(true);
    for (const auto& file : trashed) {
        if (!file.isDeleted) continue;
        std::string trashFile = trashPathFor(file);
        try {
            if (fs::exists(toPath(trashFile))) {
                fs::remove(toPath(trashFile));
            }
        } catch (const std::exception& e) {
            std::cerr << "[!] Could not delete trashed file: " << e.what() << std::endl;
        }
    }

    int removed = database.emptyTrash();

    // Clean up the trash directory itself; fs::remove only succeeds when the
    // directory is empty, so a leftover file never blocks anything.
    std::error_code ec;
    fs::remove(toPath(trashPath), ec);

    if (removed > 0) {
        std::cout << "[✓] Emptied trash (" << removed << " files permanently deleted)" << std::endl;
    } else {
        std::cout << "[i] Trash was already empty" << std::endl;
    }
    return removed;
}

// --- Backup ---

bool StorageHandler::createBackup(const std::string& backupName) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::string name;
    if (backupName.empty()) {
        std::ostringstream oss;
        oss << "backup_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
        name = oss.str();
    } else {
        name = backupName;
    }

    std::string backupDir = backupPath + "/" + name;

    try {
        if (!fs::exists(toPath(backupDir))) {
            fs::create_directories(toPath(backupDir));
        }

        // Copy all files
        auto files = database.listFiles(false);
        uint64_t totalSize = 0;
        int fileCount = 0;

        for (const auto& file : files) {
            std::string sourcePath = filesPath + "/" + file.path;
            if (fs::exists(toPath(sourcePath))) {
                std::string destPath = backupDir + "/" + file.path;
                fs::path destDir = fs::path(destPath).parent_path();
                if (!fs::exists(destDir)) {
                    fs::create_directories(destDir);
                }

                fs::copy_file(toPath(sourcePath), toPath(destPath), fs::copy_options::overwrite_existing);
                totalSize += file.size;
                fileCount++;
            }
        }

        // Record backup in database
        std::string backupDbPath = name;
        int id = database.addBackup(name, backupDbPath, totalSize, fileCount);

        if (id > 0) {
            std::cout << "[✓] Backup created: " << name << " (" << fileCount << " files, " << formatSize(totalSize) << ")" << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[✗] Backup failed: " << e.what() << std::endl;
    }

    return false;
}

std::vector<BackupRecord> StorageHandler::listBackups() {
    return database.listBackups();
}

bool StorageHandler::deleteBackup(int backupId) {
    auto backups = database.listBackups();
    for (const auto& backup : backups) {
        if (backup.id == backupId) {
            // Try to delete backup directory
            std::string backupDir = backupPath + "/" + backup.name;
            if (fs::exists(toPath(backupDir))) {
                try {
                    fs::remove_all(toPath(backupDir));
                } catch (...) {}
            }

            bool result = database.deleteBackup(backupId);
            if (result) {
                std::cout << "[✓] Deleted backup: " << backup.name << std::endl;
            }
            return result;
        }
    }

    std::cerr << "[✗] Backup not found: " << backupId << std::endl;
    return false;
}

// --- Storage Info ---

StorageStats StorageHandler::getStats() {
    return database.getStats();
}

std::string StorageHandler::getStoragePath() const {
    return storagePath;
}

std::string StorageHandler::getFilesPath() const {
    return filesPath;
}

// --- MIME Type Detection ---

std::string StorageHandler::detectMimeType(const std::string& filePath) {
    std::string ext = getExtension(filePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    static const std::map<std::string, std::string> mimeTypes = {
        // Images
        {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".png", "image/png"},
        {".gif", "image/gif"}, {".svg", "image/svg+xml"}, {".webp", "image/webp"},
        {".bmp", "image/bmp"}, {".ico", "image/x-icon"},
        // Video
        {".mp4", "video/mp4"}, {".webm", "video/webm"}, {".avi", "video/x-msvideo"},
        {".mkv", "video/x-matroska"}, {".mov", "video/quicktime"}, {".wmv", "video/x-ms-wmv"},
        // Audio
        {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".ogg", "audio/ogg"},
        {".flac", "audio/flac"}, {".aac", "audio/aac"}, {".m4a", "audio/mp4"},
        // Documents
        {".pdf", "application/pdf"}, {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
        // Text
        {".txt", "text/plain"}, {".html", "text/html"}, {".css", "text/css"},
        {".js", "text/javascript"}, {".json", "application/json"},
        {".xml", "application/xml"}, {".md", "text/markdown"},
        // Archives
        {".zip", "application/zip"}, {".tar", "application/x-tar"},
        {".gz", "application/gzip"}, {".7z", "application/x-7z-compressed"},
        // Code
        {".c", "text/x-c"}, {".cpp", "text/x-c++"}, {".h", "text/x-chdr"},
        {".java", "text/x-java"}, {".py", "text/x-python"}, {".rs", "text/x-rust"},
    };

    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        return it->second;
    }

    return "application/octet-stream";
}

// --- Utility ---

std::string StorageHandler::formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

bool StorageHandler::isValidFileName(const std::string& name) {
    if (name.empty() || name.length() > 255) return false;

    // Check for invalid characters
    const std::string invalidChars = "\\/:*?\"<>|";
    for (char c : name) {
        if (invalidChars.find(c) != std::string::npos) return false;
    }

    // Check for reserved names
    if (name == "." || name == "..") return false;

    // Windows refuses names that end with a space or a dot (they are
    // silently trimmed on disk). Leading dots are fine (e.g. ".gitignore").
    if (name.back() == ' ' || name.back() == '.') return false;

    // Check against Windows reserved device names (CON, PRN, NUL, ...).
    // The stem is compared without extension, so "nul.txt" is also refused.
    fs::path n(name);
    std::string stem = n.stem().string();
    std::string upper = stem;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    {
        static const std::vector<std::string> reserved = {
            "CON", "PRN", "AUX", "NUL",
            "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
            "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
        };
        for (const auto& r : reserved) {
            if (upper == r) return false;
        }
    }

    return true;
}

std::string StorageHandler::trashPathFor(const FileRecord& file) {
    std::string relative = file.path;
    // Avoid collisions with files that only differ by their trash id.
    return trashPath + "/" + std::to_string(file.id) + "_" +
           toPath(relative).filename().string();
}

std::string StorageHandler::generateUniquePath(const std::string& name, const std::string& folder) {
    std::string basePath = folder.empty() ? name : folder + "/" + name;

    // Check if path already exists
    if (!database.getFileByPath(basePath).has_value()) {
        return basePath;
    }

    // Generate unique path
    fs::path namePath = toPath(name);
    std::string stem = namePath.stem().string();
    std::string extension = namePath.extension().string();

    int counter = 1;
    while (true) {
        std::string newPath = folder.empty()
            ? stem + "_" + std::to_string(counter) + extension
            : folder + "/" + stem + "_" + std::to_string(counter) + extension;

        if (!database.getFileByPath(newPath).has_value()) {
            return newPath;
        }

        counter++;
    }
}

std::string StorageHandler::computeFileChecksum(const std::string& filePath) {
    // A real content hash over the file bytes (FNV-1a, 64-bit) so the stored
    // checksum actually identifies file contents instead of just size/mtime.
    try {
        fs::path wide = toPath(filePath);
        if (!fs::exists(wide)) return "";

        std::ifstream in(wide, std::ios::binary);
        if (!in.is_open()) return "";

        uint64_t hash = 14695981039346656037ull; // FNV-1a offset basis
        char buffer[8192];
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
            size_t n = static_cast<size_t>(in.gcount());
            for (size_t i = 0; i < n; ++i) {
                hash ^= static_cast<uint8_t>(buffer[i]);
                hash *= 1099511628211ull; // FNV-1a prime
            }
        }

        std::ostringstream oss;
        oss << "fnv1a:" << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    } catch (...) {
        return "";
    }
}

std::string StorageHandler::getExtension(const std::string& filePath) {
    fs::path p = toPath(filePath);
    return p.extension().string();
}

