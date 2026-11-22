// NebulaDrive CLI — the Rust port of the NebulaDrive PC app.
// Local-first: SQLite database + a plain folder of files, no servers.
//
// Usage:
//   nebuladrive [--path <data-dir>] [--version] [--help]
//   nebuladrive           starts the interactive shell

use std::io::Write;

mod db;
mod sqlite;
mod storage;
mod util;

use db::Database;
use storage::StorageHandler;

#[cfg(target_os = "windows")]
extern "C" {
    fn SetConsoleOutputCP(code_page: u32) -> u32;
    fn SetConsoleCP(code_page: u32) -> u32;
}

const VERSION: &str = "1.1.1";
const DEFAULT_DATA_DIR: &str = "./NebulaDriveData";
#[cfg(target_os = "windows")]
const CP_UTF8: u32 = 65001;

fn configure_console() {
    #[cfg(target_os = "windows")]
    unsafe {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
}

fn print_help(out: &mut dyn std::io::Write) -> std::io::Result<()> {
    write!(
        out,
        "\nNebulaDrive v{VERSION} — your private cloud\n\n\
Usage: nebuladrive [options]\n  \
  --path, -p <dir>   Data directory (default: ./NebulaDriveData)\n  \
  --version, -V      Print version and exit\n  \
  --help, -h         Print this help and exit\n\n\
Once inside the shell you can:\n  \
  status                show storage status\n  \
  list                  list all files\n  \
  import <path>         import a file\n  \
  search <query>        search files by name\n  \
  delete <id>           move a file to trash\n  \
  restore <id>          restore a file from trash\n  \
  favorite <id>         toggle favourite\n  \
  mkdir <name>          create a folder\n  \
  move <id> [folder]    move a file (empty folder = root)\n  \
  folders               list folders\n  \
  backup                create a backup\n  \
  backups               list backups\n  \
  trash                 view trash\n  \
  emptytrash            permanently delete everything in trash\n  \
  vacuum                optimize the database\n  \
  history               show recent activity\n  \
  dbinfo                show database info\n  \
  help                  show this help\n  \
  quit                  exit\n"
    )
}

fn print_banner() {
    println!("╔══════════════════════════════════════════════════╗");
    println!("║   ☁️  NebulaDrive v{VERSION:<20.2}  ║");
    println!("║   Your private cloud storage                     ║");
    println!("║   No servers. No subscriptions. Fully local.    ║");
    println!("╚══════════════════════════════════════════════════╝");
}

fn main() {
    configure_console();

    let mut data_dir = DEFAULT_DATA_DIR.to_string();
    let args = std::env::args().skip(1).collect::<Vec<String>>();
    let mut i = 0usize;
    while i < args.len() {
        let arg = &args[i];
        if arg == "--path" || arg == "-p" {
            if i + 1 >= args.len() {
                eprintln!("[!] --path requires a directory");
                std::process::exit(2);
            }
            data_dir = args[i + 1].clone();
            i += 2;
        } else if arg == "--version" || arg == "-V" {
            println!("NebulaDrive {VERSION}");
            std::process::exit(0);
        } else if arg == "--help" || arg == "-h" {
            let _ = print_help(&mut std::io::stdout());
            std::process::exit(0);
        } else if arg == "--" {
            break;
        } else if arg.starts_with('-') {
            eprintln!("[!] Unknown option: {arg}\n");
            let _ = print_help(&mut std::io::stderr());
            std::process::exit(2);
        } else {
            // A bare positional argument is treated as the data directory.
            data_dir = arg.clone();
            i += 1;
        }
    }

    let mut app = match App::new(data_dir) {
        Ok(a) => a,
        Err(e) => {
            eprintln!("Failed to start NebulaDrive: {e}");
            std::process::exit(1);
        }
    };
    if !app.initialize() {
        eprintln!("Failed to start NebulaDrive. Exiting...");
        std::process::exit(1);
    }

    app.show_status();
    app.show_menu();

    // Interactive shell — the C++ app reads lines from stdin.
    let stdin = std::io::stdin();
    let mut input = String::new();
    loop {
        print!("NebulaDrive> ");
        let _ = std::io::stdout().flush();
        input.clear();
        if stdin.read_line(&mut input).is_err() {
            break;
        }
        if input.is_empty() {
            break; // EOF
        }
        app.process_command(input.trim_end());
        if !app.running {
            break;
        }
    }
}

struct App {
    storage: StorageHandler,
    running: bool,
}

impl App {
    fn new(data_dir: String) -> Result<App, String> {
        let db_path = format!("{}/nebuladrive.db", data_dir.trim_end_matches(['/', '\\']));
        let db = Database::open(&db_path)?;
        let storage = StorageHandler::new(&data_dir, db);
        Ok(App {
            storage,
            running: true,
        })
    }

    fn initialize(&self) -> bool {
        print_banner();
        println!("\n[i] Initializing database: {}", self.storage.database.path);
        println!("[✓] Database initialized");
        match self.storage.initialize() {
            Ok(()) => {
                println!("[✓] NebulaDrive ready!\n");
                true
            }
            Err(e) => {
                eprintln!("[✗] {e}");
                false
            }
        }
    }

    // NOTE: process_command sets `running = false` for the quit command.

    // --- Views ---

    fn show_status(&self) {
        let stats = self.storage.get_stats();
        println!("\n┌──────────────────────────────────────────────┐");
        println!("│          NebulaDrive Status                  │");
        println!("├──────────────────────────────────────────────┤");
        println!("│  Status:       ✓ Running                    │");
        println!("│  Version:      {VERSION:<26} │");
        println!("│  Database:     {:<26.26} │", self.storage.database.path);
        println!("│  Files:        {:<26} │", stats.total_files);
        println!("│  Folders:      {:<26} │", stats.total_folders);
        println!("│  Storage:      {:<26} │", util::format_size(stats.total_size as u64));
        println!("│  Favorites:    {:<26} │", stats.favorite_files);
        println!("│  Trash:        {:<26} │", stats.trash_files);
        println!("│  Backups:      {:<26} │", stats.backup_count);
        println!("└──────────────────────────────────────────────┘");
    }

    fn list_files(&self) {
        let files = self.storage.database.list_files(false);
        if files.is_empty() {
            println!("[i] No files in storage. Use 'import <path>' to add files.");
            return;
        }
        println!("\n┌────┬────────────────────────────┬──────────┬────────────────┐");
        println!("│ ID │ Name                       │ Size     │ Added           │");
        println!("├────┼────────────────────────────┼──────────┼────────────────┤");
        for file in &files {
            let mut name = file.name.clone();
            if name.len() > 26 {
                name = format!("...{}", &name[name.len() - 23..]);
            }
            let fav = if file.is_favorite { " ★" } else { "" };
            println!("│ {:>3} │ {:<26}{} │ {:>8} │ {:<14} │", file.id, name, fav, util::format_size(file.size as u64), &file.created_at[0..10.min(file.created_at.len())]);
        }
        println!("└────┴────────────────────────────┴──────────┴────────────────┘");
        println!("Total: {} files", files.len());
    }

    fn list_trash(&self) {
        let files = self.storage.database.list_files(true);
        let trash: Vec<_> = files.into_iter().filter(|f| f.is_deleted).collect();
        if trash.is_empty() {
            println!("[i] Trash is empty.");
            return;
        }
        println!("\nTrash ({} files):", trash.len());
        println!("─────────────────────────────────────────────");
        for file in &trash {
            println!(
                "🗑️  [{}] {} ({}) — deleted: {}",
                file.id,
                file.name,
                util::format_size(file.size as u64),
                file.deleted_at
            );
        }
    }

    fn list_folders(&self) {
        let folders = self.storage.database.list_folders();
        if folders.is_empty() {
            println!("[i] No folders. Use 'mkdir <name>' to create one.");
            return;
        }
        println!("\nFolders:");
        println!("─────────────────────────────────────────────");
        for folder in &folders {
            println!("📁 [{}] {} (path: {})", folder.id, folder.name, folder.path);
        }
    }

    fn list_backups(&self) {
        let backups = self.storage.list_backups();
        if backups.is_empty() {
            println!("[i] No backups. Use 'backup' to create one.");
            return;
        }
        println!("\nBackups:");
        println!("─────────────────────────────────────────────");
        for backup in &backups {
            println!(
                "💾 [{}] {} ({} files, {}) — {}",
                backup.id,
                backup.name,
                backup.file_count,
                util::format_size(backup.size as u64),
                backup.created_at
            );
        }
    }

    fn show_history(&self) {
        let history = self.storage.database.get_history(20);
        if history.is_empty() {
            println!("[i] No history yet.");
            return;
        }
        println!("\nRecent activity ({} entries):", history.len());
        println!("─────────────────────────────────────────────");
        for entry in &history {
            println!("📋 [{}] {}: {}", entry.timestamp, entry.action, entry.details);
        }
    }

    fn search_files(&self, query: &str) {
        let results = self.storage.search(query);
        if results.is_empty() {
            println!("[i] No files matching '{query}'.");
            return;
        }
        println!("\nSearch results for '{query}' ({} files):", results.len());
        println!("─────────────────────────────────────────────");
        for file in &results {
            let fav = if file.is_favorite { " ★" } else { "" };
            println!(
                "🔍 [{}] {}{} ({}) — {}",
                file.id,
                file.name,
                fav,
                util::format_size(file.size as u64),
                file.mime_type
            );
        }
    }

    fn import_file(&mut self, path: &str) {
        println!("[i] Importing: {path}");
        match self.storage.import_file(path, "") {
            Ok(()) => println!("[✓] Import successful!"),
            Err(e) => println!("[✗] {e}"),
        }
    }

    fn create_backup(&mut self) {
        println!("[i] Creating backup...");
        match self.storage.create_backup(None) {
            Ok(()) => println!("[✓] Backup created!"),
            Err(e) => println!("[✗] {e}"),
        }
    }

    fn show_menu(&self) {
        println!(r#"
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
"#);
    }

    // --- Command handling ---

    fn process_command(&mut self, input: &str) {
        if input.trim().is_empty() {
            return;
        }

        let mut parts = input.splitn(2, char::is_whitespace);
        let command = parts.next().unwrap_or("").to_lowercase();
        let rest = parts.next().unwrap_or("").trim();

        match command.as_str() {
            "quit" | "exit" | "q" => {
                println!("Shutting down NebulaDrive. Goodbye! 👋");
                self.running = false;
            }
            "help" | "h" | "?" => self.show_menu(),
            "status" => self.show_status(),
            "list" | "ls" => self.list_files(),
            "trash" => self.list_trash(),
            "folders" | "dirs" => self.list_folders(),
            "backups" => self.list_backups(),
            "history" | "hist" => self.show_history(),
            "search" => {
                let query = strip_quotes(rest);
                if query.is_empty() {
                    println!("[!] Usage: search <query>");
                } else {
                    self.search_files(&query);
                }
            }
            "import" => {
                let path = strip_quotes(rest);
                if path.is_empty() {
                    println!("[!] Usage: import <file_path>");
                } else {
                    self.import_file(&path);
                }
            }
            "mkdir" | "mkdirs" => {
                let name = strip_quotes(rest);
                if name.is_empty() {
                    println!("[!] Usage: mkdir <folder_name>");
                } else {
                    match self.storage.create_folder(&name, "") {
                        Ok(()) => println!(),
                        Err(e) => println!("[✗] {e}"),
                    }
                }
            }
            "delete" | "del" | "rm" => {
                let id = parse_id(rest);
                if id.is_none() {
                    println!("[!] Usage: delete <file_id>");
                } else {
                    match self.storage.delete_file(id.unwrap()) {
                        Ok(()) => println!("[✓] File moved to trash"),
                        Err(e) => println!("[✗] {e}"),
                    }
                }
            }
            "restore" => {
                let id = parse_id(rest);
                if id.is_none() {
                    println!("[!] Usage: restore <file_id>");
                } else {
                    match self.storage.restore_file(id.unwrap()) {
                        Ok(()) => println!("[✓] File restored"),
                        Err(e) => println!("[✗] {e}"),
                    }
                }
            }
            "favorite" | "fav" | "star" => {
                let id = parse_id(rest);
                if id.is_none() {
                    println!("[!] Usage: favorite <file_id>");
                } else {
                    let id = id.unwrap();
                    match self.storage.database.toggle_favorite(id) {
                        Ok(_) => {
                            if let Some(file) = self.storage.database.get_file(id) {
                                println!(
                                    "[✓] {}: {}",
                                    if file.is_favorite { "Favorited" } else { "Unfavorited" },
                                    file.name
                                );
                            }
                        }
                        Err(e) => println!("[✗] {e}"),
                    }
                }
            }
            "move" | "mv" => {
                let mut parts = rest.splitn(2, char::is_whitespace);
                let id_str = parts.next().unwrap_or("");
                let folder = strip_quotes(parts.next().unwrap_or(""));
                if id_str.is_empty() {
                    println!("[!] Usage: move <file_id> [folder]");
                } else if let Ok(id) = id_str.parse::<i32>() {
                    match self.storage.move_file(id, &folder) {
                        Ok(()) => {}
                        Err(e) => println!("[✗] {e}"),
                    }
                } else {
                    println!("[!] Invalid ID: {id_str}");
                }
            }
            "backup" => self.create_backup(),
            "emptytrash" | "empty" => {
                match self.storage.empty_trash() {
                    Ok(count) => println!("[✓] Emptied trash: {count} files"),
                    Err(e) => println!("[✗] {e}"),
                }
            }
            "vacuum" => match self.storage.database.vacuum() {
                Ok(()) => println!("[✓] Database optimized"),
                Err(e) => println!("[✗] {e}"),
            },
            "dbinfo" => {
                println!("[i] Database: {}", self.storage.database.path);
                println!("[i] Storage: {}", self.storage.get_storage_path().display());
                println!("[i] Files dir: {}", self.storage.get_files_path().display());
            }
            _ => {
                println!("Unknown command: {command}");
                println!("Type 'help' for available commands.");
            }
        }
    }
}

fn parse_id(rest: &str) -> Option<i32> {
    rest.split_whitespace().next()?.parse::<i32>().ok()
}

fn strip_quotes(s: &str) -> String {
    let mut s = s.trim().to_string();
    while s.len() >= 2
        && ((s.starts_with('"') && s.ends_with('"')) || (s.starts_with('\'') && s.ends_with('\'')))
    {
        s = s[1..s.len() - 1].trim().to_string();
    }
    s
}
