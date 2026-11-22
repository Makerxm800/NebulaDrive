//! Minimal, dependency-free SQLite3 bindings.
//!
//! The vendored sqlite3.c (pc/vendor/sqlite/sqlite3.c) is compiled by
//! build.rs and linked into the binary — `extern "C"` here is all we need.
//! This module intentionally exposes the small surface the app uses, and
//! nothing else.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_longlong, c_void};

pub type Stmt = *mut c_void;

/// sqlite3_open_v2 flags
pub const SQLITE_OPEN_READWRITE: c_int = 0x0004;
pub const SQLITE_OPEN_CREATE: c_int = 0x0002;

pub const SQLITE_OK: c_int = 0;
pub const SQLITE_ROW: c_int = 100;
pub const SQLITE_DONE: c_int = 101;
#[allow(dead_code)]
pub const SQLITE_ERR_MISUSE: c_int = 21;

/// Destructor flag for sqlite3_bind_text / sqlite3_bind_blob / ...
/// "Do not free the memory" — we bind Rust-owned strings that outlive the
/// statement, mirroring SQLITE_STATIC in the C++ app.
pub const SQLITE_STATIC: c_int = 0; // SQLITE_TRANSIENT is different; static = no free

#[allow(dead_code)]
extern "C" {
    pub fn sqlite3_open_v2(
        filename: *const c_char,
        pp_db: *mut *mut c_void,
        flags: c_int,
        z_vfs: *const c_char,
    ) -> c_int;
    pub fn sqlite3_close(db: *mut c_void) -> c_int;
    pub fn sqlite3_exec(
        db: *mut c_void,
        sql: *const c_char,
        cb: *const std::os::raw::c_void,
        arg: *mut c_void,
        err_msg: *mut *mut c_char,
    ) -> c_int;
    pub fn sqlite3_prepare_v2(
        db: *mut c_void,
        sql: *const c_char,
        n_byte: c_int,
        stmt: *mut Stmt,
        tail: *mut *const c_char,
    ) -> c_int;
    pub fn sqlite3_bind_text(
        stmt: Stmt,
        index: c_int,
        value: *const c_char,
        n: c_int,
        dtor_flag: c_int,
    ) -> c_int;
    pub fn sqlite3_bind_text16(
        stmt: Stmt,
        index: c_int,
        value: *const u16,
        n: c_int,
        dtor_flag: c_int,
    ) -> c_int;
    pub fn sqlite3_bind_int(stmt: Stmt, index: c_int, value: c_int) -> c_int;
    pub fn sqlite3_bind_int64(stmt: Stmt, index: c_int, value: c_longlong) -> c_int;
    pub fn sqlite3_bind_null(stmt: Stmt, index: c_int) -> c_int;
    pub fn sqlite3_step(stmt: Stmt) -> c_int;
    pub fn sqlite3_finalize(stmt: Stmt) -> c_int;
    pub fn sqlite3_column_int(stmt: Stmt, index: c_int) -> c_int;
    pub fn sqlite3_column_int64(stmt: Stmt, index: c_int) -> c_longlong;
    pub fn sqlite3_column_text(stmt: Stmt, index: c_int) -> *const u8;
    pub fn sqlite3_column_text16(stmt: Stmt, index: c_int) -> *const u16;
    pub fn sqlite3_column_bytes(stmt: Stmt, index: c_int) -> c_int;
    pub fn sqlite3_errmsg(db: *mut c_void) -> *const c_char;
    pub fn sqlite3_last_insert_rowid(db: *mut c_void) -> c_longlong;
    pub fn sqlite3_changes(db: *mut c_void) -> c_int;
    pub fn sqlite3_free(ptr: *mut c_void);
}

/// RAII handle for an open SQLite database.
#[allow(dead_code)]
pub struct DbHandle {
    pub handle: *mut c_void,
}

impl DbHandle {
    /// Open or create a database at `path`. Returns the handle or a description of the failure.
    pub fn open(path: &str) -> Result<Self, String> {
        let owned = CString::new(path).map_err(|_| "path contains an embedded NUL byte")?;
        let mut db: *mut c_void = std::ptr::null_mut();
        let rc = unsafe {
            sqlite3_open_v2(
                owned.as_ptr(),
                &mut db,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                std::ptr::null(),
            )
        };
        if rc != SQLITE_OK || db.is_null() {
            let err = if !db.is_null() {
                let msg = unsafe { CStr::from_ptr(sqlite3_errmsg(db)) }.to_string_lossy().to_string();
                unsafe { sqlite3_close(db) };
                msg
            } else {
                "unknown error".to_string()
            };
            return Err(err);
        }
        Ok(Self { handle: db })
    }

    pub fn errmsg(&self) -> String {
        if self.handle.is_null() {
            return "database is not open".to_string();
        }
        unsafe { CStr::from_ptr(sqlite3_errmsg(self.handle)) }
            .to_string_lossy()
            .to_string()
    }

    /// Run a multi-statement SQL string (CREATE TABLE / PRAGMA / VACUUM ...).
    pub fn exec(&self, sql: &str) -> Result<(), String> {
        let owned = CString::new(sql).map_err(|_| "SQL contains an embedded NUL byte")?;
        let mut err: *mut c_char = std::ptr::null_mut();
        let rc = unsafe { sqlite3_exec(self.handle, owned.as_ptr(), std::ptr::null(), std::ptr::null_mut(), &mut err) };
        if rc != SQLITE_OK {
            let msg = if !err.is_null() {
                let m = unsafe { CStr::from_ptr(err) }.to_string_lossy().to_string();
                unsafe { sqlite3_free(err as *mut c_void) };
                m
            } else {
                self.errmsg()
            };
            return Err(msg);
        }
        Ok(())
    }

    pub fn prepare(&self, sql: &str) -> Result<StmtHandle, String> {
        let owned = CString::new(sql).map_err(|_| "SQL contains an embedded NUL byte")?;
        let mut stmt: Stmt = std::ptr::null_mut();
        let mut tail: *const c_char = std::ptr::null();
        let rc = unsafe {
            sqlite3_prepare_v2(self.handle, owned.as_ptr(), owned.as_bytes().len() as c_int, &mut stmt, &mut tail as *mut *const c_char)
        };
        if rc != SQLITE_OK || stmt.is_null() {
            return Err(self.errmsg());
        }
        Ok(StmtHandle { stmt, keep_alive: Vec::new() })
    }

    pub fn last_insert_rowid(&self) -> i64 {
        unsafe { sqlite3_last_insert_rowid(self.handle) }
    }

    pub fn changes(&self) -> i32 {
        unsafe { sqlite3_changes(self.handle) }
    }
}

impl Drop for DbHandle {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe { sqlite3_close(self.handle) };
            self.handle = std::ptr::null_mut();
        }
    }
}

/// RAII handle for a prepared statement.
#[allow(dead_code)]
pub struct StmtHandle {
    stmt: Stmt,
    // Buffers bound with SQLITE_STATIC — they must stay alive until the
    // statement is finalized or re-bound. Freed on drop of this struct.
    keep_alive: Vec<CString>,
}

impl StmtHandle {
    pub fn bind_text(&mut self, index: c_int, value: &str) -> Result<(), String> {
        let owned = CString::new(value).map_err(|_| "text value contains an embedded NUL byte")?;
        // SQLITE_STATIC: the statement borrows the buffer — keep it alive in
        // `keep_alive` until this handle is dropped (i.e. until finalize).
        let rc = unsafe {
            sqlite3_bind_text(
                self.stmt,
                index,
                owned.as_ptr() as *const c_char,
                owned.as_bytes().len() as c_int,
                SQLITE_STATIC,
            )
        };
        if rc != SQLITE_OK {
            return Err("failed to bind text parameter".to_string());
        }
        self.keep_alive.push(owned);
        Ok(())
    }

    pub fn bind_int(&self, index: c_int, value: c_int) -> Result<(), String> {
        let rc = unsafe { sqlite3_bind_int(self.stmt, index, value) };
        if rc != SQLITE_OK {
            return Err("failed to bind int parameter".to_string());
        }
        Ok(())
    }

    pub fn bind_int64(&self, index: c_int, value: c_longlong) -> Result<(), String> {
        let rc = unsafe { sqlite3_bind_int64(self.stmt, index, value) };
        if rc != SQLITE_OK {
            return Err("failed to bind int64 parameter".to_string());
        }
        Ok(())
    }

    #[allow(dead_code)]
    pub fn bind_null(&self, index: c_int) -> Result<(), String> {
        let rc = unsafe { sqlite3_bind_null(self.stmt, index) };
        if rc != SQLITE_OK {
            return Err("failed to bind null parameter".to_string());
        }
        Ok(())
    }

    /// Returns true if a row was produced.
    pub fn step(&self) -> Result<bool, String> {
        let rc = unsafe { sqlite3_step(self.stmt) };
        match rc {
            SQLITE_ROW => Ok(true),
            SQLITE_DONE => Ok(false),
            _ => Err(format!("sqlite3_step failed: rc={rc}")),
        }
    }

    pub fn column_int(&self, index: c_int) -> c_int {
        unsafe { sqlite3_column_int(self.stmt, index) }
    }

    pub fn column_int64(&self, index: c_int) -> c_longlong {
        unsafe { sqlite3_column_int64(self.stmt, index) }
    }

    /// Reads a text column as a String. NULL becomes an empty string.
    pub fn column_text(&self, index: c_int) -> String {
        let text = unsafe { sqlite3_column_text(self.stmt, index) };
        if text.is_null() {
            return String::new();
        }
        let len = unsafe { sqlite3_column_bytes(self.stmt, index) };
        String::from_utf8_lossy(unsafe { std::slice::from_raw_parts(text, len as usize) }).into_owned()
    }
}

impl Drop for StmtHandle {
    fn drop(&mut self) {
        if !self.stmt.is_null() {
            unsafe { sqlite3_finalize(self.stmt) };
            self.stmt = std::ptr::null_mut();
        }
    }
}
