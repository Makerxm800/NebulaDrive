// NebulaDrive CLI — the headless companion to the NebulaDrive desktop app.
// Local-first: SQLite database + a plain folder of files, no servers.
//
// Usage:
//   NebulaDriveCLI [--path <data-dir>] [--version] [--help]
//   NebulaDriveCLI           starts the interactive shell
#include "database.h"
#include "storage_handler.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

namespace fs = std::filesystem;

namespace {

constexpr const char* VERSION = "1.1.0";
constexpr const char* DEFAULT_DATA_DIR = "./NebulaDriveData";

// Make the Windows console print UTF-8 (emoji, box-drawing, ...) instead of the
// ANSI code page. On Linux/macOS this is a no-op by default.
void configureConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "");
}

void printHelp(std::ostream& out) {
    out << "\nNebulaDrive v" << VERSION << R"( — your private cloud

Usage: NebulaDriveCLI [options]
  --path, -p <dir>   Data directory (default: ./NebulaDriveData)
  --version, -V      Print version and exit
  --help, -h         Print this help and exit

Once inside the shell you can:
  status                show storage status
  list                  list all files
  import <path>         import a file
  search <query>        search files by name
  delete <id>           move a file to trash
  restore <id>          restore a file from trash
  favorite <id>         toggle favourite
  mkdir <name>          create a folder
  move <id> [folder]    move a file (empty folder = root)
  folders               list folders
  backup                create a backup
  backups               list backups
  trash                 view trash
  emptytrash            permanently delete everything in trash (bytes + records)
  vacuum                optimize the database
  history               show recent activity
  dbinfo                show database info
  help                  show this help
  quit                  exit
)";
}

void printVersion(std::ostream& out) {
    out << "NebulaDrive " << VERSION << "\n";
}

} // namespace

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------

class NebulaDriveApp {
public:
    NebulaDriveApp(const std::string& path)
        : storagePath(path), isRunning(false) {}

    ~NebulaDriveApp() {
        if (storageHandler) delete storageHandler;
        database.close();
    }

    bool initialize() {
        std::cout << "╔══════════════════════════════════════════════════╗\n";
        std::cout << "║   ☁️  NebulaDrive v" << std::setw(20 - 6) << VERSION << "  ║\n";
        std::cout << "║   Your private cloud storage                     ║\n";
        std::cout << "║   No servers. No subscriptions. Fully local.    ║\n";
        std::cout << "╚══════════════════════════════════════════════════╝\n\n";

        std::string dbPath = storagePath + "/nebuladrive.db";
        std::cout << "[i] Initializing database: " << dbPath << std::endl;

        if (!database.initialize(dbPath)) {
            std::cerr << "[✗] Failed to initialize database" << std::endl;
            return false;
        }
        std::cout << "[✓] Database initialized" << std::endl;

        storageHandler = new StorageHandler(storagePath, database);
        if (!storageHandler->initialize()) {
            std::cerr << "[✗] Failed to initialize storage handler" << std::endl;
            return false;
        }

        isRunning = true;
        std::cout << "[✓] NebulaDrive ready!\n\n";
        return true;
    }

    void showStatus() {
        if (!isRunning) { std::cout << "[!] NebulaDrive is not initialized.\n"; return; }
        auto stats = storageHandler->getStats();

        std::cout << "\n┌──────────────────────────────────────────────┐\n";
        std::cout << "│          NebulaDrive Status                  │\n";
        std::cout << "├──────────────────────────────────────────────┤\n";
        std::cout << "│  Status:       ✓ Running                    │\n";
        std::cout << "│  Version:      " << std::setw(26) << VERSION << " │\n";
        std::cout << "│  Database:     " << std::setw(26) << database.getDbPath() << " │\n";
        std::cout << "│  Files:        " << std::setw(26) << stats.totalFiles << " │\n";
        std::cout << "│  Folders:      " << std::setw(26) << stats.totalFolders << " │\n";
        std::cout << "│  Storage:      " << std::setw(26) << StorageHandler::formatSize(stats.totalSize) << " │\n";
        std::cout << "│  Favorites:    " << std::setw(26) << stats.favoriteFiles << " │\n";
        std::cout << "│  Trash:        " << std::setw(26) << stats.trashFiles << " │\n";
        std::cout << "│  Backups:      " << std::setw(26) << stats.backupCount << " │\n";
        std::cout << "└──────────────────────────────────────────────┘\n";
    }

    void listFiles() {
        if (!isRunning) return;
        auto files = database.listFiles(false);

        if (files.empty()) {
            std::cout << "[i] No files in storage. Use 'import <path>' to add files.\n";
            return;
        }

        std::cout << "\n┌────┬────────────────────────────┬──────────┬────────────────┐\n";
        std::cout << "│ ID │ Name                       │ Size     │ Added          │\n";
        std::cout << "├────┼────────────────────────────┼──────────┼────────────────┤\n";

        for (const auto& file : files) {
            std::string name = file.name;
            if (name.length() > 26) {
                name = "..." + name.substr(name.length() - 23);
            }
            std::string fav = file.isFavorite ? " ★" : "";
            std::cout << "│ " << std::setw(3) << file.id << " │ "
                      << std::setw(26) << name + fav << " │ "
                      << std::setw(8) << StorageHandler::formatSize(file.size) << " │ "
                      << std::setw(14) << file.createdAt.substr(0, 10) << " │\n";
        }

        std::cout << "└────┴────────────────────────────┴──────────┴────────────────┘\n";
        std::cout << "Total: " << files.size() << " files\n";
    }

    void listTrash() {
        if (!isRunning) return;
        auto files = database.listFiles(true);
        std::vector<FileRecord> trashFiles;
        for (const auto& file : files) {
            if (file.isDeleted) trashFiles.push_back(file);
        }

        if (trashFiles.empty()) { std::cout << "[i] Trash is empty.\n"; return; }

        std::cout << "\nTrash (" << trashFiles.size() << " files):\n";
        std::cout << "─────────────────────────────────────────────\n";
        for (const auto& file : trashFiles) {
            std::cout << "🗑️  [" << file.id << "] " << file.name
                      << " (" << StorageHandler::formatSize(file.size) << ")"
                      << " — deleted: " << file.deletedAt << "\n";
        }
    }

    void listFolders() {
        if (!isRunning) return;
        auto folders = database.listFolders();
        if (folders.empty()) {
            std::cout << "[i] No folders. Use 'mkdir <name>' to create one.\n";
            return;
        }
        std::cout << "\nFolders:\n";
        std::cout << "─────────────────────────────────────────────\n";
        for (const auto& folder : folders) {
            std::cout << "📁 [" << folder.id << "] " << folder.name
                      << " (path: " << folder.path << ")\n";
        }
    }

    void listBackups() {
        if (!isRunning) return;
        auto backups = storageHandler->listBackups();
        if (backups.empty()) {
            std::cout << "[i] No backups. Use 'backup' to create one.\n";
            return;
        }
        std::cout << "\nBackups:\n";
        std::cout << "─────────────────────────────────────────────\n";
        for (const auto& backup : backups) {
            std::cout << "💾 [" << backup.id << "] " << backup.name
                      << " (" << backup.fileCount << " files, "
                      << StorageHandler::formatSize(backup.size) << ")"
                      << " — " << backup.createdAt << "\n";
        }
    }

    void showHistory(int limit = 20) {
        if (!isRunning) return;
        auto history = database.getHistory(limit);
        if (history.empty()) { std::cout << "[i] No history yet.\n"; return; }
        std::cout << "\nRecent activity (" << history.size() << " entries):\n";
        std::cout << "─────────────────────────────────────────────\n";
        for (const auto& entry : history) {
            std::cout << "📋 [" << entry.timestamp << "] " << entry.action
                      << ": " << entry.details << "\n";
        }
    }

    void searchFiles(const std::string& query) {
        if (!isRunning) return;
        auto results = storageHandler->search(query);
        if (results.empty()) {
            std::cout << "[i] No files matching '" << query << "'.\n";
            return;
        }
        std::cout << "\nSearch results for '" << query << "' (" << results.size() << " files):\n";
        std::cout << "─────────────────────────────────────────────\n";
        for (const auto& file : results) {
            std::string fav = file.isFavorite ? " ★" : "";
            std::cout << "🔍 [" << file.id << "] " << file.name << fav
                      << " (" << StorageHandler::formatSize(file.size) << ")"
                      << " — " << file.mimeType << "\n";
        }
    }

    void importFile(const std::string& path) {
        if (!isRunning) return;
        std::cout << "[i] Importing: " << path << std::endl;
        if (storageHandler->importFile(path)) {
            std::cout << "[✓] Import successful!\n";
        } else {
            std::cout << "[✗] Import failed!\n";
        }
    }

    void createBackup() {
        if (!isRunning) return;
        std::cout << "[i] Creating backup...\n";
        if (storageHandler->createBackup()) {
            std::cout << "[✓] Backup created!\n";
        } else {
            std::cout << "[✗] Backup failed!\n";
        }
    }

    void showMenu() {
        std::cout << R"(
╔══════════════════════════════════════════════════╗
║           NebulaDrive Commands                   ║
╠══════════════════════════════════════════════════╣
║  Files:                                          ║
║    list                    - List all files      ║
║    import <path>           - Import a file       ║
║    search <query>          - Search files        ║
║    delete <id>             - Move to trash       ║
║    restore <id>            - Restore from trash  ║
║    favorite <id>           - Toggle favorite     ║
║  Folders:                                        ║
║    mkdir <name>            - Create folder       ║
║    move <id> [folder]      - Move file           ║
║    folders                 - List folders        ║
║  Backup:                                         ║
║    backup                  - Create backup       ║
║    backups                 - List backups        ║
║  Trash:                                          ║
║    trash                   - View trash          ║
║    emptytrash              - Delete everything   ║
║  Info:                                           ║
║    status                  - Show status         ║
║    history                 - Show history        ║
║    help                    - Show this menu      ║
║    quit                    - Exit NebulaDrive    ║
╚══════════════════════════════════════════════════╝
)";
    }

    bool is_running() const { return isRunning; }

    void processCommand(const std::string& input) {
        if (input.empty()) return;

        std::istringstream iss(input);
        std::string command;
        iss >> command;
        std::transform(command.begin(), command.end(), command.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (command == "quit" || command == "exit" || command == "q") {
            std::cout << "Shutting down NebulaDrive. Goodbye! 👋\n";
            isRunning = false;
            return;
        }
        if (!isRunning) {
            std::cout << "[!] NebulaDrive is not initialized.\n";
            return;
        }

        if (command == "help" || command == "h" || command == "?") {
            showMenu();
        } else if (command == "status") {
            showStatus();
        } else if (command == "list" || command == "ls") {
            listFiles();
        } else if (command == "trash") {
            listTrash();
        } else if (command == "folders" || command == "dirs") {
            listFolders();
        } else if (command == "backups") {
            listBackups();
        } else if (command == "history" || command == "hist") {
            showHistory();
        } else if (command == "search") {
            std::string query;
            std::getline(iss, query);
            if (!query.empty() && query[0] == ' ') query = query.substr(1);
            if (query.empty()) {
                std::cout << "[!] Usage: search <query>\n";
            } else {
                searchFiles(query);
            }
        } else if (command == "import") {
            std::string path;
            std::getline(iss, path);
            if (!path.empty() && path[0] == ' ') path = path.substr(1);
            if (path.empty()) {
                std::cout << "[!] Usage: import <file_path>\n";
            } else {
                importFile(path);
            }
        } else if (command == "mkdir" || command == "mkdirs") {
            std::string name;
            std::getline(iss, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            if (name.empty()) {
                std::cout << "[!] Usage: mkdir <folder_name>\n";
            } else if (storageHandler->createFolder(name)) {
                std::cout << "[✓] Folder created: " << name << "\n";
            }
        } else if (command == "delete" || command == "del" || command == "rm") {
            std::string idStr;
            iss >> idStr;
            if (idStr.empty()) {
                std::cout << "[!] Usage: delete <file_id>\n";
            } else {
                try {
                    int id = std::stoi(idStr);
                    if (storageHandler->deleteFile(id)) {
                        std::cout << "[✓] File moved to trash\n";
                    }
                } catch (...) {
                    std::cout << "[!] Invalid ID: " << idStr << "\n";
                }
            }
        } else if (command == "restore") {
            std::string idStr;
            iss >> idStr;
            if (idStr.empty()) {
                std::cout << "[!] Usage: restore <file_id>\n";
            } else {
                try {
                    int id = std::stoi(idStr);
                    if (storageHandler->restoreFile(id)) {
                        std::cout << "[✓] File restored\n";
                    }
                } catch (...) {
                    std::cout << "[!] Invalid ID: " << idStr << "\n";
                }
            }
        } else if (command == "favorite" || command == "fav" || command == "star") {
            std::string idStr;
            iss >> idStr;
            if (idStr.empty()) {
                std::cout << "[!] Usage: favorite <file_id>\n";
            } else {
                try {
                    int id = std::stoi(idStr);
                    if (database.toggleFavorite(id)) {
                        auto file = database.getFile(id);
                        if (file) {
                            std::cout << "[✓] "
                                      << (file->isFavorite ? "Unfavorited" : "Favorited")
                                      << ": " << file->name << "\n";
                        }
                    }
                } catch (...) {
                    std::cout << "[!] Invalid ID: " << idStr << "\n";
                }
            }
        } else if (command == "move" || command == "mv") {
            std::string idStr;
            iss >> idStr;
            std::string folder;
            std::getline(iss, folder);
            // Trim leading whitespace and optional surrounding quotes, e.g.
            //   move 1 "My Folder"
            size_t start = folder.find_first_not_of(" \"'");
            folder = (start == std::string::npos) ? "" : folder.substr(start);
            size_t end = folder.find_last_not_of(" \"'");
            folder = (end == std::string::npos) ? "" : folder.substr(0, end + 1);
            while (folder.size() >= 2 && folder.front() == '"' && folder.back() == '"') {
                folder = folder.substr(1, folder.size() - 2);
            }
            if (idStr.empty()) {
                std::cout << "[!] Usage: move <file_id> [folder]\n";
            } else {
                try {
                    int id = std::stoi(idStr);
                    // An empty folder moves the file to the root.
                    if (storageHandler->moveFile(id, folder)) {
                        std::cout << "[✓] File moved to "
                                  << (folder.empty() ? "root" : "'" + folder + "'") << "\n";
                    }
                } catch (...) {
                    std::cout << "[!] Invalid ID: " << idStr << "\n";
                }
            }
        } else if (command == "backup") {
            createBackup();
        } else if (command == "emptytrash" || command == "empty") {
            int count = storageHandler->emptyTrash();
            std::cout << "[✓] Emptied trash: " << count << " files\n";
        } else if (command == "vacuum") {
            if (database.vacuum()) {
                std::cout << "[✓] Database optimized\n";
            } else {
                std::cout << "[✗] Optimization failed\n";
            }
        } else if (command == "dbinfo") {
            std::cout << "[i] Database: " << database.getDbPath() << "\n";
            std::cout << "[i] Storage: " << storagePath << "\n";
            std::cout << "[i] Files dir: " << storageHandler->getFilesPath() << "\n";
        } else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }
    }

private:
    std::string storagePath;
    Database& database = Database::instance();
    StorageHandler* storageHandler = nullptr;
    bool isRunning = false;
};

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    configureConsole();

    std::string dataDir = DEFAULT_DATA_DIR;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--path" || arg == "-p") {
            if (i + 1 >= argc) {
                std::cerr << "[!] --path requires a directory\n";
                return 2;
            }
            dataDir = argv[++i];
        } else if (arg == "--version" || arg == "-V") {
            printVersion(std::cout);
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            printHelp(std::cout);
            return 0;
        } else if (arg == "--") {
            break;  // ignore anything after --
        } else if (arg.rfind("-", 0) == 0) {
            std::cerr << "[!] Unknown option: " << arg << "\n\n";
            printHelp(std::cerr);
            return 2;
        } else {
            // A bare positional argument is treated as the data directory.
            dataDir = arg;
        }
    }

    NebulaDriveApp app(dataDir);
    if (!app.initialize()) {
        std::cerr << "Failed to start NebulaDrive. Exiting...\n";
        return 1;
    }

    app.showStatus();
    app.showMenu();

    std::string command;
    while (true) {
        std::cout << "NebulaDrive> ";
        if (!std::getline(std::cin, command)) {
            break;
        }
        app.processCommand(command);
        if (!app.is_running()) break;
    }

    return 0;
}
