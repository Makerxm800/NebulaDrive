//! Small pure helpers: timestamps, sizes, MIME detection, validation, checksums.

use std::time::{SystemTime, UNIX_EPOCH};

/// FNV-1a 64-bit checksum of a file, formatted like the C++ app:
/// "fnv1a:<16 lowercase hex chars>".
pub fn compute_file_checksum(path: &std::path::Path) -> String {
    use std::io::Read;
    let mut file = match std::fs::File::open(path) {
        Ok(f) => f,
        Err(_) => return String::new(),
    };
    let mut hasher = Fnv1a::new();
    let mut buffer = [0u8; 8192];
    loop {
        let n = match file.read(&mut buffer) {
            Ok(n) => n,
            Err(_) => return String::new(),
        };
        if n == 0 {
            break;
        }
        hasher.update(&buffer[..n]);
    }
    format!("fnv1a:{:016x}", hasher.digest())
}

pub struct Fnv1a {
    hash: u64,
}

impl Fnv1a {
    pub fn new() -> Self {
        Fnv1a { hash: 14695981039346656037u64 }
    }
    pub fn update(&mut self, bytes: &[u8]) {
        for &b in bytes {
            self.hash ^= b as u64;
            self.hash = self.hash.wrapping_mul(1099511628211u64);
        }
    }
    pub fn digest(&self) -> u64 {
        self.hash
    }
}

/// Formats a byte count like the C++ app: "B", "KB", "MB", "GB", "TB" — 2 decimals.
pub fn format_size(bytes: u64) -> String {
    const UNITS: [&str; 5] = ["B", "KB", "MB", "GB", "TB"];
    let mut size = bytes as f64;
    let mut unit = 0usize;
    while size >= 1024.0 && unit < UNITS.len() - 1 {
        size /= 1024.0;
        unit += 1;
    }
    format!("{:.2} {}", size, UNITS[unit])
}

/// Validates a file name for safe use on disk. Mirrors the C++ app:
///   - not empty, not longer than 255 chars
///   - no \ / : * ? " < > |
///   - not "." or ".."
///   - must not end with a space or a dot
///   - not a Windows reserved device name (stem, case-insensitive)
pub fn is_valid_file_name(name: &str) -> bool {
    if name.is_empty() || name.len() > 255 {
        return false;
    }
    const INVALID: &[char] = &['\\', '/', ':', '*', '?', '"', '<', '>', '|'];
    for c in name.chars() {
        if INVALID.contains(&c) {
            return false;
        }
    }
    if name == "." || name == ".." {
        return false;
    }
    let last = name.chars().last().unwrap_or(' ');
    if last == ' ' || last == '.' {
        return false;
    }
    // stem = name without its extension ("nul.txt" -> "nul", leading dots stripped)
    let stem = match name.rfind('.') {
        Some(i) if i > 0 => &name[..i],
        _ => name,
    };
    let stem = stem.trim_start_matches('.');
    let upper = stem.to_uppercase();
    const RESERVED: [&str; 22] = [
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    ];
    for r in RESERVED.iter() {
        if upper == *r {
            return false;
        }
    }
    true
}

/// Maps a file extension (with leading dot, e.g. ".png") to its MIME type.
pub fn detect_mime_type(file_path: &str) -> String {
    let ext = get_extension(file_path);
    let ext = ext.to_lowercase();
    let mime = match ext.as_str() {
        ".jpg" | ".jpeg" => "image/jpeg",
        ".png" => "image/png",
        ".gif" => "image/gif",
        ".svg" => "image/svg+xml",
        ".webp" => "image/webp",
        ".bmp" => "image/bmp",
        ".ico" => "image/x-icon",
        ".mp4" => "video/mp4",
        ".webm" => "video/webm",
        ".avi" => "video/x-msvideo",
        ".mkv" => "video/x-matroska",
        ".mov" => "video/quicktime",
        ".wmv" => "video/x-ms-wmv",
        ".mp3" => "audio/mpeg",
        ".wav" => "audio/wav",
        ".ogg" => "audio/ogg",
        ".flac" => "audio/flac",
        ".aac" => "audio/aac",
        ".m4a" => "audio/mp4",
        ".pdf" => "application/pdf",
        ".doc" => "application/msword",
        ".docx" => "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
        ".xls" => "application/vnd.ms-excel",
        ".xlsx" => "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
        ".ppt" => "application/vnd.ms-powerpoint",
        ".pptx" => "application/vnd.openxmlformats-officedocument.presentationml.presentation",
        ".txt" => "text/plain",
        ".html" => "text/html",
        ".css" => "text/css",
        ".js" => "text/javascript",
        ".json" => "application/json",
        ".xml" => "application/xml",
        ".md" => "text/markdown",
        ".zip" => "application/zip",
        ".tar" => "application/x-tar",
        ".gz" => "application/gzip",
        ".7z" => "application/x-7z-compressed",
        ".c" => "text/x-c",
        ".cpp" => "text/x-c++",
        ".h" => "text/x-chdr",
        ".java" => "text/x-java",
        ".py" => "text/x-python",
        ".rs" => "text/x-rust",
        _ => "application/octet-stream",
    };
    mime.to_string()
}

pub fn get_extension(file_path: &str) -> String {
    // Use the last '.' in the file name component, not the whole path.
    let file_name = file_path.rsplit(['/', '\\']).next().unwrap_or(file_path);
    match file_name.rfind('.') {
        Some(i) if i > 0 => file_name[i..].to_string(),
        _ => String::new(),
    }
}

/// Current time as "YYYY-MM-DD HH:MM:SS" in UTC.
/// (The C++ app uses local time; we use UTC so builds behave identically
/// regardless of host timezone.)
pub fn current_timestamp() -> String {
    let now = SystemTime::now();
    let secs = now
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    let (year, month, day, hour, minute, second) = civil_from_unix_secs(secs);
    format!("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", year, month, day, hour, minute, second)
}

/// Timestamp used in backup folder names ("backup_YYYYMMDD_HHMMSS").
pub fn backup_name(prefix: &str) -> String {
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0);
    let (year, month, day, hour, minute, second) = civil_from_unix_secs(secs);
    format!("{}_{:04}{:02}{:02}_{:02}{:02}{:02}", prefix, year, month, day, hour, minute, second)
}

/// Howard Hinnant's civil_from_days algorithm — deterministic UTC conversion.
fn civil_from_unix_secs(secs: u64) -> (i64, i64, i64, i64, i64, i64) {
    let days = secs / 86400;
    let seconds_of_day = secs % 86400;
    let hour = seconds_of_day / 3600;
    let minute = (seconds_of_day % 3600) / 60;
    let second = seconds_of_day % 60;

    let z = days as i64 + 719468;
    let era = if z >= 0 { z } else { z - 146096 } / 146097;
    let doe = (z - era * 146097) as u64;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let year = yoe as i64 + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = if mp < 10 { mp + 3 } else { mp - 9 };
    let year = if month <= 2 { year + 1 } else { year };

    (year, month as i64, day as i64, hour as i64, minute as i64, second as i64)
}

/// Like the C++ app: escape LIKE wildcards in the pattern before LIKE-matching it.
pub fn like_escape(input: &str) -> String {
    let mut out = String::with_capacity(input.len() + 8);
    for c in input.chars() {
        if c == '\\' || c == '%' || c == '_' {
            out.push('\\');
        }
        out.push(c);
    }
    out
}
