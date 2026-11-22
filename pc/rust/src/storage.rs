//! File/folder operations on disk + the database, mirroring pc/storage_handler.cpp.

use crate::db::{BackupRecord, Database, FileRecord, StorageStats};
use crate::util::{
    compute_file_checksum, detect_mime_type, current_timestamp, format_size,
    get_extension, is_valid_file_name,
};
use std::path::{Path, PathBuf};

#[allow(dead_code)] // full API kept for future GUI work
pub struct StorageHandler {
    pub storage_path: PathBuf,
    pub files_path: PathBuf,
    pub trash_path: PathBuf,
    pub backup_path: PathBuf,
    pub database: Database,
}

/// Like the C++ app: pick a name that does not collide with an existing one.
/// If `name` is already in use (in the given folder), produces
/// `<stem>_1<ext>`, `<stem>_2<ext>`, ...
/// `exclude_path` is ignored during the uniqueness check (used by move_file
/// to keep the file's own current record from making it collide with itself).
pub fn generate_unique_path(
    name: &str,
    folder: &str,
    exclude_path: Option<&str>,
    database: &Database,
) -> String {
    let base_path = if folder.is_empty() {
        name.to_string()
    } else {
        format!("{folder}/{name}")
    };
    if base_path != exclude_path.unwrap_or("") && database.get_file_by_path(&base_path).is_none() {
        return base_path;
    }
    let (stem, ext) = get_path_stem(name);
    let mut counter = 1u32;
    loop {
        let new_path = if folder.is_empty() {
            format!("{stem}_{counter}{ext}")
        } else {
            format!("{folder}/{stem}_{counter}{ext}")
        };
        if new_path != exclude_path.unwrap_or("") && database.get_file_by_path(&new_path).is_none() {
            return new_path;
        }
        counter += 1;
    }
}

pub fn get_path_stem(file_name: &str) -> (&str, String) {
    if let Some(dot) = file_name.rfind('.') {
        if dot > 0 {
            return (&file_name[..dot], file_name[dot..].to_string());
        }
    }
    (file_name, String::new())
}

#[allow(dead_code)]
impl StorageHandler {
    
    pub fn new(storage_path: &str, database: Database) -> StorageHandler {
        let root = PathBuf::from(storage_path);
        StorageHandler {
            files_path: root.join("files"),
            trash_path: root.join("trash"),
            backup_path: root.join("backups"),
            storage_path: root,
            database,
        }
    }

    pub fn initialize(&self) -> Result<(), String> {
        for dir in [&self.files_path, &self.trash_path, &self.backup_path] {
            if !dir.exists() {
                std::fs::create_dir_all(dir)
                    .map_err(|e| format!("failed to create {}: {e}", dir.display()))?;
                println!("[✓] Created directory: {}", dir.display());
            }
        }
        Ok(())
    }

    // --- File operations ---

    pub fn import_file(&mut self, source_path: &str, destination_folder: &str) -> Result<(), String> {
        let source = PathBuf::from(source_path);
        if !source.exists() {
            return Err(format!("source file not found: {source_path}"));
        }
        if !source.is_file() {
            return Err(format!("source is not a file: {source_path}"));
        }
        let file_name = source
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_else(|| "file".to_string());

        let relative_path = generate_unique_path(&file_name, destination_folder, None, &self.database);
        let dest_full = self.files_path.join(&relative_path);

        if let Some(parent) = dest_full.parent() {
            std::fs::create_dir_all(parent)
                .map_err(|e| format!("failed to create destination directory: {e}"))?;
        }

        std::fs::copy(&source, &dest_full).map_err(|e| format!("failed to copy file: {e}"))?;

        let size = std::fs::metadata(&dest_full).map(|m| m.len()).unwrap_or(0);
        let mime_type = detect_mime_type(&file_name);
        let checksum = compute_file_checksum(&dest_full);

        let id = self.database.add_file(
            &file_name,
            &relative_path,
            destination_folder,
            &mime_type,
            size as i64,
            &checksum,
        )?;

        if id <= 0 {
            return Err("database did not return a file id".to_string());
        }

        println!("[✓] Imported: {file_name} ({})", format_size(size));
        Ok(())
    }

    pub fn move_file(&mut self, file_id: i32, destination_folder: &str) -> Result<(), String> {
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;

        // Moving into the folder it already lives in is a no-op.
        if destination_folder == file.parent_folder {
            println!(
                "[!] {} is already in {}",
                file.name,
                if destination_folder.is_empty() { "root".to_string() } else { destination_folder.to_string() }
            );
            return Ok(());
        }

        let source = self.files_path.join(&file.path);
        if !source.exists() {
            return Err("source file not found on disk".to_string());
        }

        let relative_path = generate_unique_path(&file.name, destination_folder, Some(&file.path), &self.database);
        let dest_full = self.files_path.join(&relative_path);

        if let Some(parent) = dest_full.parent() {
            std::fs::create_dir_all(parent)
                .map_err(|e| format!("failed to create destination directory: {e}"))?;
        }

        std::fs::rename(&source, &dest_full)
            .or_else(|_| {
                std::fs::copy(&source, &dest_full)?;
                std::fs::remove_file(&source)
            })
            .map_err(|e| format!("failed to move file: {e}"))?;

        let parents_ok = self.database.update_file(
            file_id,
            &file.name,
            &relative_path,
            file.size,
            destination_folder,
        )?;

        if !parents_ok {
            // Restore the move if the record update failed.
            let _ = std::fs::rename(&dest_full, &source);
            return Err("database update failed".to_string());
        }

        println!(
            "[✓] Moved: {} to {}",
            file.name,
            if destination_folder.is_empty() { "root".to_string() } else { destination_folder.to_string() }
        );
        Ok(())
    }

    pub fn rename_file(&mut self, file_id: i32, new_name: &str) -> Result<(), String> {
        if !is_valid_file_name(new_name) {
            return Err("invalid file name".to_string());
        }
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;

        let old_path = self.files_path.join(&file.path);
        if !old_path.exists() {
            return Err("file not found on disk".to_string());
        }

        // Keep the original extension unless the caller supplied the matching one.
        let extension = get_extension(&old_path.to_string_lossy());
        let mut stem = new_name.to_string();
        if !extension.is_empty()
            && stem.len() >= extension.len()
            && stem.ends_with(&extension)
        {
            stem = stem[..stem.len() - extension.len()].to_string();
        }
        let final_name = format!("{stem}{extension}");

        let new_relative_path = if file.parent_folder.is_empty() {
            final_name.clone()
        } else {
            format!("{}/{final_name}", file.parent_folder)
        };
        let new_full_path = self.files_path.join(&new_relative_path);

        if new_full_path.exists() {
            return Err("a file with that name already exists".to_string());
        }

        std::fs::rename(&old_path, &new_full_path)
            .map_err(|e| format!("failed to rename: {e}"))?;

        let ok = self.database.update_file(file_id, &final_name, &new_relative_path, file.size, &file.parent_folder)?;
        if !ok {
            let _ = std::fs::rename(&new_full_path, &old_path);
            return Err("database update failed".to_string());
        }

        println!("[✓] Renamed: {} -> {}", file.name, final_name);
        Ok(())
    }

    pub fn delete_file(&mut self, file_id: i32) -> Result<(), String> {
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;

        let source = self.files_path.join(&file.path);
        if source.exists() {
            let trash_full = self.trash_path_for(&file);
            if let Some(parent) = trash_full.parent() {
                let _ = std::fs::create_dir_all(parent);
            }
            let _ = std::fs::rename(&source, &trash_full);
        }

        let ok = self.database.soft_delete(file_id)?;
        if !ok {
            return Err("failed to flag file as deleted".to_string());
        }
        Ok(())
    }

    pub fn permanently_delete_file(&mut self, file_id: i32) -> Result<(), String> {
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;

        for candidate in [self.files_path.join(&file.path), self.trash_path_for(&file)] {
            if candidate.exists() {
                let _ = std::fs::remove_file(&candidate);
            }
        }

        let ok = self.database.hard_delete(file_id)?;
        if !ok {
            return Err("database delete failed".to_string());
        }

        println!("[✓] Permanently deleted: {}", file.name);
        Ok(())
    }

    pub fn restore_file(&mut self, file_id: i32) -> Result<(), String> {
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;

        let trash_file = self.trash_path_for(&file);
        let live_file = self.files_path.join(&file.path);
        if trash_file.exists() {
            if let Some(parent) = live_file.parent() {
                let _ = std::fs::create_dir_all(parent);
            }
            let _ = std::fs::rename(&trash_file, &live_file);
        }

        let ok = self.database.restore_file(file_id)?;
        if !ok {
            return Err("failed to restore file record".to_string());
        }
        println!("[✓] Restored: {}", file.name);
        Ok(())
    }

    pub fn export_file(&mut self, file_id: i32, destination_path: &str) -> Result<(), String> {
        let file = self
            .database
            .get_file(file_id)
            .ok_or_else(|| format!("file not found: {file_id}"))?;
        let source = self.files_path.join(&file.path);
        if !source.exists() {
            return Err("file not found on disk".to_string());
        }
        let dest = PathBuf::from(destination_path);
        if let Some(parent) = dest.parent() {
            if !parent.as_os_str().is_empty() && !parent.exists() {
                std::fs::create_dir_all(parent)
                    .map_err(|e| format!("failed to create destination directory: {e}"))?;
            }
        }
        std::fs::copy(&source, &dest).map_err(|e| format!("failed to export: {e}"))?;
        println!("[✓] Exported: {} to {}", file.name, destination_path);
        Ok(())
    }

    // --- Folders ---

    pub fn create_folder(&mut self, name: &str, parent_path: &str) -> Result<(), String> {
        if !is_valid_file_name(name) {
            return Err("invalid folder name".to_string());
        }
        let id = self.database.create_folder(name, parent_path)?;
        if id <= 0 {
            return Err("folder already exists or creation failed".to_string());
        }
        let folder_dir = if parent_path.is_empty() {
            name.to_string()
        } else {
            format!("{parent_path}/{name}")
        };
        let _ = std::fs::create_dir_all(self.files_path.join(&folder_dir));
        println!("[✓] Created folder: {name}");
        Ok(())
    }

    pub fn rename_folder(&mut self, folder_id: i32, new_name: &str) -> Result<(), String> {
        if !is_valid_file_name(new_name) {
            return Err("invalid folder name".to_string());
        }
        let folder = self
            .database
            .get_folder_by_id(folder_id)
            .ok_or_else(|| format!("folder not found: {folder_id}"))?;

        let old_path = self.files_path.join(&folder.path);
        let parent = folder.parent_folder.clone();
        let new_path = if parent.is_empty() {
            new_name.to_string()
        } else {
            format!("{parent}/{new_name}")
        };
        let new_full = self.files_path.join(&new_path);

        if new_full.exists() {
            return Err("a folder with that name already exists".to_string());
        }

        // Move the whole tree — files and sub-folders travel with the parent.
        if old_path.exists() {
            std::fs::rename(&old_path, &new_full)
                .map_err(|e| format!("failed to rename folder: {e}"))?;
        }

        // Update folder descendants' records.
        let old_prefix = folder.path.clone();
        let new_prefix = new_path.clone();

        let folders = self.database.list_folders();
        for child in folders {
            if child.id == folder_id {
                continue;
            }
            if let Some(suffix) = child.path.strip_prefix(&old_prefix) {
                if suffix.starts_with('/') {
                    let updated = format!("{new_prefix}{suffix}");
                    let parent_of_child = child.parent_folder.clone();
                    self.database.rename_folder_path(
                        child.id,
                        &updated,
                        if parent_of_child == old_prefix { &new_prefix } else { &parent_of_child },
                    );
                }
            }
        }

        // Update file records inside the folder (the bytes already moved with
        // the folder rename above — only the records need the new paths).
        let files = self.database.list_files(true);
        for file in files {
            if let Some(suffix) = file.parent_folder.strip_prefix(&old_prefix) {
                if suffix.starts_with('/') {
                    let updated_parent = format!("{new_prefix}{suffix}");
                    let updated_path = format!("{updated_parent}/{}", file.name);
                    self.database.rename_file_path(file.id, &updated_path, &updated_parent);
                }
            }
        }

        self.database.rename_folder_path(folder_id, &new_path, &parent);
        let _ = self.database.add_history("rename", &new_path, &format!("Renamed folder from: {old_prefix}"));
        println!("[✓] Renamed folder: {} -> {}", folder.name, new_name);
        Ok(())
    }

    pub fn delete_folder(&mut self, folder_id: i32) -> Result<(), String> {
        let folder = self
            .database
            .get_folder_by_id(folder_id)
            .ok_or_else(|| format!("folder not found: {folder_id}"))?;
        let prefix = folder.path.clone();

        // Remove every file inside the folder.
        let files = self.database.list_files(true);
        for file in files {
            let parent = &file.parent_folder;
            let inside = parent == &prefix
                || (parent.len() > prefix.len()
                    && parent.starts_with(&prefix)
                    && parent.as_str()[prefix.len()..].starts_with('/'));
            if !inside {
                continue;
            }
            let full = self.files_path.join(&file.path);
            if full.exists() {
                let _ = std::fs::remove_file(&full);
            }
            let _ = self.database.hard_delete(file.id);
        }

        // Remove sub-folder records.
        let folders = self.database.list_folders();
        for child in folders {
            if child.id == folder_id {
                continue;
            }
            if child.path.len() > prefix.len()
                && child.path.starts_with(&prefix)
                && child.path.as_str()[prefix.len()..].starts_with('/')
            {
                let _ = self.database.delete_folder(child.id);
            }
        }

        let folder_dir = self.files_path.join(&prefix);
        if folder_dir.exists() {
            let _ = std::fs::remove_dir_all(&folder_dir);
        }

        self.database.delete_folder(folder_id);
        let _ = self.database.add_history("delete", &prefix, &format!("Deleted folder: {}", folder.name));
        println!("[✓] Deleted folder: {}", folder.name);
        Ok(())
    }

    pub fn list_folder(&mut self, folder_path: &str) -> Vec<FileRecord> {
        self.database.list_files_by_folder(folder_path, false)
    }

    pub fn search(&self, query: &str) -> Vec<FileRecord> {
        self.database.search_files(query)
    }

    // --- Trash ---

    pub fn trash_path_for(&self, file: &FileRecord) -> PathBuf {
        let filename = PathBuf::from(&file.path)
            .file_name()
            .map(|n| n.to_string_lossy().to_string())
            .unwrap_or_else(|| "file".to_string());
        self.trash_path.join(format!("{}_{}", file.id, filename))
    }

    pub fn empty_trash(&mut self) -> Result<i32, String> {
        let trashed = self.database.list_files(true);
        for file in trashed {
            if !file.is_deleted {
                continue;
            }
            let trash_file = self.trash_path_for(&file);
            if trash_file.exists() {
                let _ = std::fs::remove_file(&trash_file);
            }
        }

        let removed = self.database.empty_trash()?;

        // Clean up the trash directory itself if it's empty.
        let _ = std::fs::remove_dir(&self.trash_path);

        if removed > 0 {
            println!("[✓] Emptied trash ({removed} files permanently deleted)");
        } else {
            println!("[i] Trash was already empty");
        }
        Ok(removed)
    }

    // --- Backup ---

    pub fn create_backup(&mut self, backup_name: Option<&str>) -> Result<(), String> {
        let name = match backup_name {
            Some(n) if !n.is_empty() => n.to_string(),
            _ => crate::util::backup_name("backup"),
        };
        let backup_dir = self.backup_path.join(&name);

        if !backup_dir.exists() {
            std::fs::create_dir_all(&backup_dir)
                .map_err(|e| format!("failed to create backup directory: {e}"))?;
        }

        let files = self.database.list_files(false);
        let mut total_size = 0i64;
        let mut file_count = 0i32;

        for file in files {
            let source = self.files_path.join(&file.path);
            if source.exists() {
                let dest = backup_dir.join(&file.path);
                if let Some(parent) = dest.parent() {
                    std::fs::create_dir_all(parent)
                        .map_err(|e| format!("failed to create backup parent: {e}"))?;
                }
                if let Err(e) = std::fs::copy(&source, &dest) {
                    return Err(format!("failed to copy {}: {e}", file.name));
                }
                total_size += file.size;
                file_count += 1;
            }
        }

        let id = self.database.add_backup(&name, &name, total_size, file_count)?;
        if id <= 0 {
            return Err("backup record could not be written".to_string());
        }

        println!(
            "[✓] Backup created: {name} ({file_count} files, {})",
            format_size(total_size as u64)
        );
        Ok(())
    }

    pub fn list_backups(&self) -> Vec<BackupRecord> {
        self.database.list_backups()
    }

    pub fn delete_backup(&mut self, backup_id: i32) -> Result<(), String> {
        let backups = self.database.list_backups();
        let backup = backups
            .iter()
            .find(|b| b.id == backup_id)
            .ok_or_else(|| format!("backup not found: {backup_id}"))?;
        let backup_dir = self.backup_path.join(&backup.name);
        if backup_dir.exists() {
            let _ = std::fs::remove_dir_all(&backup_dir);
        }
        let ok = self.database.delete_backup(backup_id);
        if ok {
            println!("[✓] Deleted backup: {}", backup.name);
        } else {
            return Err("database delete failed".to_string());
        }
        Ok(())
    }

    // --- Info ---

    pub fn get_stats(&self) -> StorageStats {
        self.database.get_stats()
    }

    pub fn get_storage_path(&self) -> &Path {
        &self.storage_path
    }

    pub fn get_files_path(&self) -> &Path {
        &self.files_path
    }

    pub fn current_timestamp() -> String {
        current_timestamp()
    }

    pub fn stats_display(&self) -> String {
        let s = self.get_stats();
        format!(
            "files={} total={} favorites={} trash={} backups={} images={} audio={} video={}",
            s.total_files,
            format_size(s.total_size as u64),
            s.favorite_files,
            s.trash_files,
            s.backup_count,
            s.image_files,
            s.audio_files,
            s.video_files
        )
    }
}
