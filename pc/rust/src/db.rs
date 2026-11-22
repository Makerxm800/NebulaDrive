//! Local SQLite storage — schema and queries, mirroring pc/database.cpp.
//!
//! Same tables, same semantics, same operations. Timestamps are UTC
//! ("YYYY-MM-DD HH:MM:SS").

use crate::sqlite::{DbHandle, StmtHandle};
use crate::util::{current_timestamp, like_escape};

/// Returns true when the statement reached SQLITE_DONE (i.e. the write completed).
fn is_done(stmt: &StmtHandle) -> Result<bool, String> {
    Ok(!stmt.step()?)
}

#[derive(Debug, Clone, Default)]
#[allow(dead_code)] // full API kept for future GUI work
pub struct FileRecord {
    pub id: i32,
    pub name: String,
    pub path: String, // relative path within storage ("" is invalid here)
    pub parent_folder: String,
    pub mime_type: String,
    pub size: i64,
    pub checksum: String,
    pub created_at: String,
    pub modified_at: String,
    pub tags: String,
    pub is_favorite: bool,
    pub is_deleted: bool,
    pub deleted_at: String,
}

#[derive(Debug, Clone, Default)]
#[allow(dead_code)]
pub struct BackupRecord {
    pub id: i32,
    pub name: String,
    pub path: String,
    pub size: i64,
    pub file_count: i32,
    pub created_at: String,
    pub status: String,
    pub description: String,
}

#[derive(Debug, Clone, Default)]
#[allow(dead_code)]
pub struct HistoryRecord {
    pub id: i32,
    pub action: String,
    pub file_path: String,
    pub details: String,
    pub timestamp: String,
}

#[derive(Debug, Clone, Default)]
#[allow(dead_code)]
pub struct StorageStats {
    pub total_files: i64,
    pub total_folders: i64,
    pub total_size: i64,
    pub trash_files: i64,
    pub trash_size: i64,
    pub favorite_files: i64,
    pub backup_count: i64,
    pub video_files: i64,
    pub image_files: i64,
    pub audio_files: i64,
}

const SCHEMA: &str = "
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    path TEXT NOT NULL UNIQUE,
    parent_folder TEXT DEFAULT '',
    mime_type TEXT DEFAULT 'application/octet-stream',
    size INTEGER DEFAULT 0,
    checksum TEXT DEFAULT '',
    created_at TEXT NOT NULL,
    modified_at TEXT NOT NULL,
    tags TEXT DEFAULT '',
    is_favorite INTEGER DEFAULT 0,
    is_deleted INTEGER DEFAULT 0,
    deleted_at TEXT DEFAULT ''
);
CREATE TABLE IF NOT EXISTS folders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    path TEXT NOT NULL UNIQUE,
    parent_path TEXT DEFAULT '',
    created_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS backups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    path TEXT NOT NULL,
    size INTEGER DEFAULT 0,
    file_count INTEGER DEFAULT 0,
    created_at TEXT NOT NULL,
    status TEXT DEFAULT 'completed',
    description TEXT DEFAULT ''
);
CREATE TABLE IF NOT EXISTS history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    action TEXT NOT NULL,
    file_path TEXT DEFAULT '',
    details TEXT DEFAULT '',
    timestamp TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_files_path ON files(path);
CREATE INDEX IF NOT EXISTS idx_files_name ON files(name);
CREATE INDEX IF NOT EXISTS idx_files_deleted ON files(is_deleted);
CREATE INDEX IF NOT EXISTS idx_files_favorite ON files(is_favorite);
CREATE INDEX IF NOT EXISTS idx_files_checksum ON files(checksum);
CREATE INDEX IF NOT EXISTS idx_history_timestamp ON history(timestamp);
";

#[allow(dead_code)]
pub struct Database {
    pub db: DbHandle,
    pub path: String,
}

#[allow(dead_code)]
impl Database {
    pub fn open(path: &str) -> Result<Database, String> {
        // Ensure the parent directory exists.
        if let Some(parent) = std::path::Path::new(path).parent() {
            if !parent.as_os_str().is_empty() && !parent.exists() {
                std::fs::create_dir_all(parent)
                    .map_err(|e| format!("failed to create database directory {}: {e}", parent.display()))?;
            }
        }
        let db = DbHandle::open(path)?;
        let dbself = Database { db, path: path.to_string() };
        dbself.create_tables()?;
        Ok(dbself)
    }

    pub fn create_tables(&self) -> Result<(), String> {
        self.db.exec(SCHEMA)?;
        // WAL mode + NORMAL sync for performance (mirrors the C++ app).
        let _ = self.db.exec("PRAGMA journal_mode=WAL;");
        let _ = self.db.exec("PRAGMA synchronous=NORMAL;");
        let _ = self.db.exec("PRAGMA foreign_keys=ON;");
        // Default settings (INSERT OR IGNORE).
        let _ = self.db.exec("INSERT OR IGNORE INTO settings (key, value) VALUES ('version', '1.1.0');");
        let _ = self.db.exec("INSERT OR IGNORE INTO settings (key, value) VALUES ('theme', 'dark');");
        let _ = self.db.exec("INSERT OR IGNORE INTO settings (key, value) VALUES ('language', 'en');");
        Ok(())
    }

    // --- Files ---

    pub fn add_file(
        &mut self,
        name: &str,
        path: &str,
        parent_folder: &str,
        mime_type: &str,
        size: i64,
        checksum: &str,
    ) -> Result<i32, String> {
        let timestamp = current_timestamp();
        let mut stmt = self.db.prepare(
            "INSERT INTO files (name, path, parent_folder, mime_type, size, checksum, created_at, modified_at) \
             VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
        )?;
        stmt.bind_text(1, name)?;
        stmt.bind_text(2, path)?;
        stmt.bind_text(3, parent_folder)?;
        stmt.bind_text(4, mime_type)?;
        stmt.bind_int64(5, size)?;
        stmt.bind_text(6, checksum)?;
        stmt.bind_text(7, &timestamp)?;
        stmt.bind_text(8, &timestamp)?;
        let done = is_done(&stmt)?;
        let id = self.db.last_insert_rowid() as i32;
        if done {
            let _ = self.add_history("upload", path, &format!("File uploaded: {name}"));
            Ok(id)
        } else {
            Err("failed to add file record".to_string())
        }
    }

    pub fn update_file(
        &mut self,
        id: i32,
        name: &str,
        path: &str,
        size: i64,
        parent_folder: &str,
    ) -> Result<bool, String> {
        let timestamp = current_timestamp();
        let mut stmt = self.db.prepare(
            "UPDATE files SET name = ?, path = ?, size = ?, modified_at = ?, parent_folder = ? WHERE id = ?;",
        )?;
        stmt.bind_text(1, name)?;
        stmt.bind_text(2, path)?;
        stmt.bind_int64(3, size)?;
        stmt.bind_text(4, &timestamp)?;
        stmt.bind_text(5, parent_folder)?;
        stmt.bind_int(6, id)?;
        let done = is_done(&stmt)?;
        if done {
            let _ = self.add_history("update", path, &format!("File updated: {name}"));
            Ok(true)
        } else {
            Ok(false)
        }
    }

    pub fn get_file(&self, id: i32) -> Option<FileRecord> {
        let stmt = self.db.prepare("SELECT * FROM files WHERE id = ?;").ok()?;
        stmt.bind_int(1, id).ok()?;
        if let Ok(true) = stmt.step() {
            return Some(read_file_row(&stmt));
        }
        None
    }

    pub fn get_file_by_path(&self, path: &str) -> Option<FileRecord> {
        let mut stmt = self.db.prepare("SELECT * FROM files WHERE path = ?;").ok()?;
        stmt.bind_text(1, path).ok()?;
        if let Ok(true) = stmt.step() {
            return Some(read_file_row(&stmt));
        }
        None
    }

    pub fn list_files(&self, include_deleted: bool) -> Vec<FileRecord> {
        let mut sql = String::from("SELECT * FROM files WHERE 1=1");
        if !include_deleted {
            sql.push_str(" AND is_deleted = 0");
        }
        sql.push_str(" ORDER BY name ASC;");
        let mut files = Vec::new();
        if let Ok(stmt) = self.db.prepare(&sql) {
            loop {
                match stmt.step() {
                    Ok(true) => files.push(read_file_row(&stmt)),
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        files
    }

    pub fn list_files_by_folder(&self, parent_folder: &str, include_deleted: bool) -> Vec<FileRecord> {
        let mut sql = String::from("SELECT * FROM files WHERE parent_folder = ?");
        if !include_deleted {
            sql.push_str(" AND is_deleted = 0");
        }
        sql.push_str(" ORDER BY name ASC;");
        let mut files = Vec::new();
        if let Ok(mut stmt) = self.db.prepare(&sql) {
            let _ = stmt.bind_text(1, parent_folder);
            loop {
                match stmt.step() {
                    Ok(true) => files.push(read_file_row(&stmt)),
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        files
    }

    pub fn list_favorite_files(&self) -> Vec<FileRecord> {
        let mut files = Vec::new();
        if let Ok(stmt) = self.db.prepare("SELECT * FROM files WHERE is_favorite = 1 AND is_deleted = 0 ORDER BY name ASC;") {
            loop {
                match stmt.step() {
                    Ok(true) => files.push(read_file_row(&stmt)),
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        files
    }

    pub fn search_files(&self, pattern: &str) -> Vec<FileRecord> {
        let search_pattern = format!("%{}%", like_escape(pattern));
        let mut files = Vec::new();
        if let Ok(mut stmt) = self.db.prepare("SELECT * FROM files WHERE name LIKE ? ESCAPE '\\' AND is_deleted = 0 ORDER BY name ASC;") {
            let _ = stmt.bind_text(1, &search_pattern);
            loop {
                match stmt.step() {
                    Ok(true) => files.push(read_file_row(&stmt)),
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        files
    }

    pub fn toggle_favorite(&mut self, id: i32) -> Result<bool, String> {
        let stmt = self.db.prepare("UPDATE files SET is_favorite = NOT is_favorite WHERE id = ?;")?;
        stmt.bind_int(1, id)?;
        let done = is_done(&stmt)?;
        if done {
            if let Some(file) = self.get_file(id) {
                let _ = self.add_history("favorite", &file.path, &format!("Toggled favorite for: {}", file.name));
            }
        }
        Ok(done)
    }

    pub fn soft_delete(&mut self, id: i32) -> Result<bool, String> {
        let timestamp = current_timestamp();
        let mut stmt = self.db.prepare("UPDATE files SET is_deleted = 1, deleted_at = ? WHERE id = ?;")?;
        stmt.bind_text(1, &timestamp)?;
        stmt.bind_int(2, id)?;
        let done = is_done(&stmt)?;
        if done {
            if let Some(file) = self.get_file(id) {
                let _ = self.add_history("delete", &file.path, &format!("Moved to trash: {}", file.name));
            }
        }
        Ok(done)
    }

    pub fn hard_delete(&mut self, id: i32) -> Result<bool, String> {
        let stmt = self.db.prepare("DELETE FROM files WHERE id = ?;")?;
        stmt.bind_int(1, id)?;
        let done = is_done(&stmt)?;
        if done {
            let _ = self.add_history("permanent_delete", "", &format!("Permanently deleted file ID: {id}"));
        }
        Ok(done)
    }

    pub fn restore_file(&mut self, id: i32) -> Result<bool, String> {
        let stmt = self.db.prepare("UPDATE files SET is_deleted = 0, deleted_at = '' WHERE id = ?;")?;
        stmt.bind_int(1, id)?;
        let done = is_done(&stmt)?;
        if done {
            if let Some(file) = self.get_file(id) {
                let _ = self.add_history("restore", &file.path, &format!("Restored from trash: {}", file.name));
            }
        }
        Ok(done)
    }

    pub fn empty_trash(&mut self) -> Result<i32, String> {
        let stmt = self.db.prepare("DELETE FROM files WHERE is_deleted = 1;")?;
        let _ = stmt.step().map_err(|e| e.to_string())?;
        let count = self.db.changes();
        if count > 0 {
            let _ = self.add_history("empty_trash", "", &format!("Emptied trash: {count} files"));
        }
        Ok(count)
    }

    // --- Folders ---

    pub fn create_folder(&mut self, name: &str, parent_path: &str) -> Result<i32, String> {
        let timestamp = current_timestamp();
        let path = if parent_path.is_empty() { name.to_string() } else { format!("{parent_path}/{name}") };
        let mut stmt = self.db.prepare("INSERT INTO folders (name, path, parent_path, created_at) VALUES (?, ?, ?, ?);")?;
        stmt.bind_text(1, name)?;
        stmt.bind_text(2, &path)?;
        stmt.bind_text(3, parent_path)?;
        stmt.bind_text(4, &timestamp)?;
        let done = is_done(&stmt)?;
        let id = self.db.last_insert_rowid() as i32;
        if done {
            let _ = self.add_history("create_folder", &path, &format!("Created folder: {name}"));
            Ok(id)
        } else {
            Ok(-1)
        }
    }

    pub fn list_folders(&self) -> Vec<FileRecord> {
        let mut folders = Vec::new();
        if let Ok(stmt) = self.db.prepare("SELECT * FROM folders ORDER BY name ASC;") {
            loop {
                match stmt.step() {
                    Ok(true) => {
                        let mut rec = FileRecord::default();
                        rec.id = stmt.column_int(0);
                        rec.name = stmt.column_text(1);
                        rec.path = stmt.column_text(2);
                        rec.parent_folder = stmt.column_text(3);
                        rec.created_at = stmt.column_text(4);
                        folders.push(rec);
                    }
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        folders
    }

    pub fn get_folder_by_path(&self, path: &str) -> Option<FileRecord> {
        let mut stmt = self.db.prepare("SELECT * FROM folders WHERE path = ?;").ok()?;
        stmt.bind_text(1, path).ok()?;
        if let Ok(true) = stmt.step() {
            let mut rec = FileRecord::default();
            rec.id = stmt.column_int(0);
            rec.name = stmt.column_text(1);
            rec.path = stmt.column_text(2);
            rec.parent_folder = stmt.column_text(3);
            rec.created_at = stmt.column_text(4);
            return Some(rec);
        }
        None
    }

    pub fn get_folder_by_id(&self, id: i32) -> Option<FileRecord> {
        let stmt = self.db.prepare("SELECT * FROM folders WHERE id = ?;").ok()?;
        stmt.bind_int(1, id).ok()?;
        if let Ok(true) = stmt.step() {
            let mut rec = FileRecord::default();
            rec.id = stmt.column_int(0);
            rec.name = stmt.column_text(1);
            rec.path = stmt.column_text(2);
            rec.parent_folder = stmt.column_text(3);
            rec.created_at = stmt.column_text(4);
            return Some(rec);
        }
        None
    }

    pub fn delete_folder(&mut self, id: i32) -> bool {
        let Ok(stmt) = self.db.prepare("DELETE FROM folders WHERE id = ?;") else { return false };
        let _ = stmt.bind_int(1, id);
        matches!(stmt.step(), Ok(false))
    }

    pub fn rename_folder_path(&mut self, id: i32, path: &str, parent_path: &str) -> bool {
        let name = path.rsplit('/').next().unwrap_or(path).to_string();
        let Ok(mut stmt) = self.db.prepare("UPDATE folders SET path = ?, parent_path = ?, name = ? WHERE id = ?;") else { return false };
        let _ = stmt.bind_text(1, path);
        let _ = stmt.bind_text(2, parent_path);
        let _ = stmt.bind_text(3, &name);
        let _ = stmt.bind_int(4, id);
        matches!(stmt.step(), Ok(false))
    }

    pub fn update_folder_parent(&mut self, id: i32, parent_path: &str) -> bool {
        let Ok(mut stmt) = self.db.prepare("UPDATE folders SET parent_path = ? WHERE id = ?;") else { return false };
        let _ = stmt.bind_text(1, parent_path);
        let _ = stmt.bind_int(2, id);
        matches!(stmt.step(), Ok(false))
    }

    pub fn rename_file_path(&mut self, id: i32, path: &str, parent_folder: &str) -> bool {
        let Ok(mut stmt) = self.db.prepare("UPDATE files SET path = ?, parent_folder = ? WHERE id = ?;") else { return false };
        let _ = stmt.bind_text(1, path);
        let _ = stmt.bind_text(2, parent_folder);
        let _ = stmt.bind_int(3, id);
        matches!(stmt.step(), Ok(false))
    }

    // --- Settings ---

    pub fn get_setting(&self, key: &str) -> String {
        let Ok(mut stmt) = self.db.prepare("SELECT value FROM settings WHERE key = ?;") else { return String::new() };
        if stmt.bind_text(1, key).is_err() {
            return String::new();
        }
        match stmt.step() {
            Ok(true) => stmt.column_text(0),
            _ => String::new(),
        }
    }

    pub fn set_setting(&mut self, key: &str, value: &str) -> bool {
        let Ok(mut stmt) = self.db.prepare("INSERT INTO settings (key, value) VALUES (?, ?) ON CONFLICT(key) DO UPDATE SET value = excluded.value;") else { return false };
        let _ = stmt.bind_text(1, key);
        let _ = stmt.bind_text(2, value);
        matches!(stmt.step(), Ok(false))
    }

    // --- Backups ---

    pub fn add_backup(&mut self, name: &str, path: &str, size: i64, file_count: i32) -> Result<i32, String> {
        let timestamp = current_timestamp();
        let mut stmt = self.db.prepare(
            "INSERT INTO backups (name, path, size, file_count, created_at, status) VALUES (?, ?, ?, ?, ?, 'completed');",
        )?;
        stmt.bind_text(1, name)?;
        stmt.bind_text(2, path)?;
        stmt.bind_int64(3, size)?;
        stmt.bind_int(4, file_count)?;
        stmt.bind_text(5, &timestamp)?;
        let done = is_done(&stmt)?;
        let id = self.db.last_insert_rowid() as i32;
        if done {
            let _ = self.add_history("backup", path, &format!("Backup created: {name}"));
            Ok(id)
        } else {
            Ok(-1)
        }
    }

    pub fn list_backups(&self) -> Vec<BackupRecord> {
        let mut backups = Vec::new();
        if let Ok(stmt) = self.db.prepare("SELECT * FROM backups ORDER BY created_at DESC;") {
            loop {
                match stmt.step() {
                    Ok(true) => {
                        backups.push(BackupRecord {
                            id: stmt.column_int(0),
                            name: stmt.column_text(1),
                            path: stmt.column_text(2),
                            size: stmt.column_int64(3),
                            file_count: stmt.column_int(4),
                            created_at: stmt.column_text(5),
                            status: stmt.column_text(6),
                            description: stmt.column_text(7),
                        });
                    }
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        backups
    }

    pub fn delete_backup(&mut self, id: i32) -> bool {
        let Ok(stmt) = self.db.prepare("DELETE FROM backups WHERE id = ?;") else { return false };
        let _ = stmt.bind_int(1, id);
        matches!(stmt.step(), Ok(false))
    }

    // --- History ---

    pub fn add_history(&mut self, action: &str, file_path: &str, details: &str) -> Result<i32, String> {
        let timestamp = current_timestamp();
        let mut stmt = self.db.prepare("INSERT INTO history (action, file_path, details, timestamp) VALUES (?, ?, ?, ?);")?;
        stmt.bind_text(1, action)?;
        stmt.bind_text(2, file_path)?;
        stmt.bind_text(3, details)?;
        stmt.bind_text(4, &timestamp)?;
        let done = stmt.step().map_err(|e| e.to_string())?;
        let id = self.db.last_insert_rowid() as i32;
        let _ = done;
        Ok(id)
    }

    pub fn get_history(&self, limit: i32) -> Vec<HistoryRecord> {
        let mut history = Vec::new();
        if let Ok(stmt) = self.db.prepare("SELECT * FROM history ORDER BY timestamp DESC LIMIT ?;") {
            let _ = stmt.bind_int(1, limit);
            loop {
                match stmt.step() {
                    Ok(true) => {
                        history.push(HistoryRecord {
                            id: stmt.column_int(0),
                            action: stmt.column_text(1),
                            file_path: stmt.column_text(2),
                            details: stmt.column_text(3),
                            timestamp: stmt.column_text(4),
                        });
                    }
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        history
    }

    // --- Statistics ---

    pub fn get_stats(&self) -> StorageStats {
        let mut stats = StorageStats::default();

        // Total files (not deleted)
        if let Ok(stmt) = self.db.prepare("SELECT COUNT(*), COALESCE(SUM(size), 0) FROM files WHERE is_deleted = 0;") {
            if let Ok(true) = stmt.step() {
                stats.total_files = stmt.column_int64(0);
                stats.total_size = stmt.column_int64(1);
            }
        }
        // Total folders
        if let Ok(stmt) = self.db.prepare("SELECT COUNT(*) FROM folders;") {
            if let Ok(true) = stmt.step() {
                stats.total_folders = stmt.column_int64(0);
            }
        }
        // Trash files
        if let Ok(stmt) = self.db.prepare("SELECT COUNT(*), COALESCE(SUM(size), 0) FROM files WHERE is_deleted = 1;") {
            if let Ok(true) = stmt.step() {
                stats.trash_files = stmt.column_int64(0);
                stats.trash_size = stmt.column_int64(1);
            }
        }
        // Favorite files
        if let Ok(stmt) = self.db.prepare("SELECT COUNT(*) FROM files WHERE is_favorite = 1 AND is_deleted = 0;") {
            if let Ok(true) = stmt.step() {
                stats.favorite_files = stmt.column_int64(0);
            }
        }
        // Backup count
        if let Ok(stmt) = self.db.prepare("SELECT COUNT(*) FROM backups;") {
            if let Ok(true) = stmt.step() {
                stats.backup_count = stmt.column_int64(0);
            }
        }
        // Media counts
        if let Ok(stmt) = self.db.prepare("SELECT mime_type FROM files WHERE is_deleted = 0;") {
            loop {
                match stmt.step() {
                    Ok(true) => {
                        let mime = stmt.column_text(0);
                        if mime.starts_with("video/") {
                            stats.video_files += 1;
                        } else if mime.starts_with("image/") {
                            stats.image_files += 1;
                        } else if mime.starts_with("audio/") {
                            stats.audio_files += 1;
                        }
                    }
                    Ok(false) => break,
                    Err(_) => break,
                }
            }
        }
        stats
    }

    pub fn vacuum(&self) -> Result<(), String> {
        self.db.exec("VACUUM;")
    }
}

fn read_file_row(stmt: &StmtHandle) -> FileRecord {
    FileRecord {
        id: stmt.column_int(0),
        name: stmt.column_text(1),
        path: stmt.column_text(2),
        parent_folder: stmt.column_text(3),
        mime_type: stmt.column_text(4),
        size: stmt.column_int64(5),
        checksum: stmt.column_text(6),
        created_at: stmt.column_text(7),
        modified_at: stmt.column_text(8),
        tags: stmt.column_text(9),
        is_favorite: stmt.column_int(10) != 0,
        is_deleted: stmt.column_int(11) != 0,
        deleted_at: stmt.column_text(12),
    }
}
