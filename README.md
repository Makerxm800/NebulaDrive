# ☁️ NebulaCloud — Your Private Cloud, Offline‑First

Sorry Some stuff are not steel finished and yeah its made by ai but i made it myself as my thinking and ik how to make it human by how i use the ai to make it work for me so thats wye

> **One HTML file. Zero servers. Unlimited control.**

---

<div align="center">

![Version](https://img.shields.io/badge/version-BETA_2.0-blue?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Platform](https://img.shields.io/badge/platform-browser-lightgrey?style=for-the-badge)
![Server](https://img.shields.io/badge/server-none%20required-brightgreen?style=for-the-badge)
![Storage](https://img.shields.io/badge/storage-OPFS%20%2B%20IndexedDB-orange?style=for-the-badge)

</div>

---

## 📖 Table of Contents

1. [What is NebulaCloud?](#-what-is-nebulacloud)
2. [Key Highlights](#-key-highlights)
3. [Full Feature List](#-full-feature-list)
4. [Getting Started](#-getting-started)
5. [User Interface Walkthrough](#-user-interface-walkthrough)
6. [Keyboard Shortcuts](#-keyboard-shortcuts)
7. [Technical Architecture](#-technical-architecture)
8. [Browser Support & Performance](#-browser-support--performance)
9. [Backup & Restore Explained](#-backup--restore-explained)
10. [Diagnostics & Repair](#-diagnostics--repair)
11. [Customisation](#-customisation)
12. [Advanced Usage](#-advanced-usage)
13. [Troubleshooting & FAQ](#-troubleshooting--faq)
14. [Contributing](#-contributing)
15. [License](#-license)
16. [Roadmap](#-roadmap)

---

## 🌟 What is NebulaCloud?

NebulaCloud is a **fully‑featured personal cloud** that runs completely inside your web browser.  
It uses modern web technologies (IndexedDB and OPFS) to store your files, folders, and account data **locally** – nothing is ever sent to any server.

- **No sign‑ups, no subscriptions, no hidden costs.**  
- **Your data stays on your device – always.**  
- **A premium, glass‑morphism interface** that rivals paid cloud services.  
- **Built‑in media player** with advanced controls.  
- **20+ themes**, including premium options.

---

## ✨ Key Highlights

| 🔒 **Absolute Privacy** | ⚡ **Blazing‑Fast Performance** | 🎨 **Stunning Design** | 🎬 **Built‑in Media** |
|--------------------------|--------------------------------|-------------------------|------------------------|
| 100% local storage – nothing leaves your device | OPFS + IndexedDB for high‑speed binary storage | Glass‑morphism UI with 20+ premium themes | Audio & video player with Picture‑in‑Picture |
| No tracking, no analytics, no third‑party calls | Chunked large files (>25MB) for reliability | Responsive, smooth, and delightfully animated | Quality dashboard, speed control, and repeat modes |
| Guest mode for instant access | Batch operations (select, invert, bulk delete) | Custom folder icons (emoji or image upload) | YouTube integration – auto‑detect video IDs |

---

## 📦 Full Feature List

### 1. File Management

| Feature | Description |
|---------|-------------|
| **Upload** | Drag‑and‑drop anywhere on the page, or click the upload area. Progress bar shows real‑time status. |
| **Download** | Click the download button on any file – saves directly to your device. |
| **Folder Creation** | Create nested folders with the **+ New Folder** button. |
| **Rename** | Rename any file or folder via the action buttons or the “More” menu. |
| **Move** | Move files and folders to any other folder using the **Move** action – a modal lets you search and pick the destination. |
| **Delete** | Moves items to the **Trash** (soft delete). You can restore or permanently delete from there. |
| **⭐ Favorites** | Star any file or folder – they appear in the dedicated **Favorites** tab for quick access. |
| **🗑️ Trash** | Deleted items stay here until you restore them or empty the trash permanently. |
| **🔍 Search** | Instant search across all files and folders – results show the full path. |
| **Multiple Views** | Choose from **Small Grid**, **Medium Grid**, **Large Grid**, **Gallery**, **List**, or **Compact** – all with smooth transitions. |
| **Sorting** | Sort by **Name**, **Size**, or **Date** – in ascending or descending order. |
| **Batch Operations** | Enable selection mode, then select multiple items. You can **invert selection**, **deselect all**, or **delete selected** (moves to trash). |
| **Folder Icons** | Right‑click (or tap) the folder icon badge on any folder to open the icon picker – choose from 40+ emojis or upload your own image (PNG, JPG, SVG). |

### 2. Premium Media Player

| Feature | Description |
|---------|-------------|
| **Playback Controls** | Play/Pause, seek (drag the progress bar), volume, playback speed (0.5× to 2×), ±5 second skip buttons. |
| **Repeat Modes** | **Off**, **All** (repeat the entire queue), **One** (repeat the current track). Visual indicators show the active mode. |
| **Quality Dashboard** | Click the gear icon to see resolution, bitrate (in kbps), file size, format, and duration (for video/audio). |
| **Mini Player** | When you close the full player, a mini player stays at the bottom of the screen – you can continue browsing while music plays. |
| **Picture‑in‑Picture** | For videos, you can pop the player out into a floating window that stays on top of other applications (Chrome/Edge). |
| **Immersive Fullscreen** | Click the **Full** button to enter a cinema‑like fullscreen mode that hides all UI elements (auto‑hides after 3 seconds of inactivity). |
| **Native Fullscreen** | Use the **Cinema** button to enter true native fullscreen (like YouTube). |
| **YouTube Integration** | If a video file is named like `[VIDEO_ID].mp4` (e.g. `[dQw4w9WgXcQ].mp4`), a YouTube badge appears – clicking it opens the video on YouTube. |

### 3. Themes

| 🌙 Standard Themes | ⭐ Premium Themes (toggle in Settings) |
|--------------------|---------------------------------------|
| Dark Nebula        | Midnight Gold ✦                       |
| Light Aurora       | Velvet Rose ✦                         |
| Azure Abyss        | Emerald Noir ✦                        |
| Crimson Horizon    | Platinum Frost ✦                      |
| Nordic Frost       | Crimson Ember ✦                       |
| Dracula Noir       | Sapphire Royale ✦                     |
| Solarized Dusk     | Obsidian Chrome ✦                     |
|                    | Aurora Borealis ✦                     |
|                    | Cosmic Latte ✦                        |
|                    | Forest Whisper ✦                      |
|                    | Rose Gold ✦                           |
|                    | Nebula Rose ✦                         |

**All themes** are applied instantly – no page reload required.

### 4. Backup & Restore

| Feature | Description |
|---------|-------------|
| **📦 Export ZIP** | Downloads a real `.zip` file containing **all your files** (preserving folder structure) plus a `manifest.json` with metadata (users, folders, file list). |
| **📥 Import ZIP** | Upload a backup ZIP to fully restore your account, files, and folders – overwrites current data (with confirmation). |
| **📄 JSON Export** | Exports account credentials, recent accounts, folders, and history as a JSON file – useful for migrating to another browser. |
| **📥 JSON Import** | Import a JSON backup to restore account data and settings. |

### 5. Diagnostics & Repair

| Feature | Description |
|---------|-------------|
| **🔍 File Integrity Scan** | Scans every file record against your browser’s actual storage – detects **missing** (no data) or **corrupted** (size mismatch) files. |
| **🔧 Repair Individual Files** | For any file flagged as missing/corrupt, use the **Repair** action to re‑upload a replacement. |
| **🧹 Remove Broken Records** | In the diagnostics results, you can remove all broken records at once (cleans up dangling metadata). |
| **📊 Live Storage Stats** | The Settings panel shows real‑time counts for folders, files, total storage used, video count, image count, and disk usage (if supported). |

### 6. Security & Privacy

| Aspect | Implementation |
|--------|----------------|
| **Data Storage** | 100% local – no data ever sent to any server. |
| **Authentication** | Username + password (stored locally in IndexedDB). |
| **Guest Mode** | Instant access without creating an account – perfect for quick use. |
| **File Storage** | Uses **OPFS** (Origin Private File System) when available – fast, secure, and isolated. |
| **Fallback** | Automatically falls back to IndexedDB chunking when OPFS is not supported. |
| **Chunking** | Files >25MB are split into reliable chunks for safe storage. |

---

## 🚀 Getting Started

### 1. Open the File
Download `index.html` and open it in your browser – **no server, no installation**.

### 2. Create an Account
Enter a username and password. Everything stays local.  
**Tip:** Use **Continue as Guest** for instant access – no credentials needed.

### 3. Start Uploading
- **Drag & Drop** – drag files from your file explorer onto the page.  
- **Click to Browse** – click the upload area to open the file picker.

### 4. Organise Your Content
- Create folders with the **+ New Folder** button.  
- Star favourites with the ⭐ icon.  
- Switch views and sorting from the toolbar.  
- Move files/folders using the **Move** action.

### 5. Preview & Play
- Click any file to preview it – images, audio, video, text, PDF, CSV.  
- Use the built‑in media player for audio and video.  
- Click the gear icon for quality details.

### 6. Customise
- Open **Settings** (⚙️ gear icon) to change themes, view storage stats, run diagnostics, or manage your account.  
- Toggle **Premium Themes** to see the full collection.

---

## 🖥️ User Interface Walkthrough

### Main Screen
- **Top Navbar** – logo, tabs (Files, Favorites, History, Trash), user avatar, and settings/switch account buttons.  
- **Breadcrumb** – shows your current folder path; click any part to navigate instantly.  
- **Toolbar** – view mode, sort options, favorites filter, and search bar.  
- **Upload Area** – drag‑and‑drop zone.  
- **Files Grid** – displays folders and files with their icons, names, sizes, and action buttons.  

### Action Buttons on Cards
- **⭐ Star** – toggle favorite.  
- **👁️ View** – preview the file (image, media, text, etc.).  
- **⬇️ Save** – download the file.  
- **✏️ Rename** – rename the item.  
- **📂 Move** – open the move modal to select a new parent folder.  
- **🗑️ Delete** – move to trash.  
- **⋯ More** – additional options (repair, etc.).  

### Selection Mode
Enable selection mode via the **Select** toggle. Then check boxes appear on each card – you can select multiple, invert, deselect, or delete all selected.

### Media Player
- Full player opens automatically when you click an audio/video file.  
- Mini player appears when you close the full player.  
- Use keyboard shortcuts for control.

### Settings Panel
- **Storage Overview** – real‑time stats.  
- **Account Controls** – rename, switch, log out.  
- **Backup** – export/import ZIP or JSON.  
- **Diagnostics** – scan and repair.  
- **Themes** – choose from all themes (premium toggle).  
- **Danger Zone** – clear all files (moves to trash).

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Context | Action |
|----------|---------|--------|
| `Space`  | Media Player active | Play / Pause |
| `➡️ Arrow Right` | Media Player active | Next track |
| `⬅️ Arrow Left` | Media Player active | Previous track |
| `F`       | Media Player active | Toggle immersive fullscreen |
| `Escape`  | Media Player active | Close fullscreen / Minimize player |

---

## 🛠️ Technical Architecture

### 🗂️ Storage Layers
┌─────────────────────────────────────────┐
│ NebulaCloud │
├─────────────────────────────────────────┤
│ ┌───────────────────────────────────┐ │
│ │ 🗄️ IndexedDB (Metadata) │ │
│ │ Users · Folders · History · Trash│ │
│ │ File metadata · Folder icons │ │
│ │ Custom icons (base64) │ │
│ └───────────────────────────────────┘ │
│ ┌───────────────────────────────────┐ │
│ │ 💾 OPFS (File Content) │ │
│ │ Fast, secure binary storage │ │
│ │ (fallback: IndexedDB chunks) │ │
│ └───────────────────────────────────┘ │
│ ┌───────────────────────────────────┐ │
│ │ 📦 JSZip (Backup/Restore) │ │
│ │ ZIP export/import functionality │ │
│ └───────────────────────────────────┘ │
└─────────────────────────────────────────┘

### Data Flow
1. **Upload** → file is read as ArrayBuffer → stored in OPFS (or chunked in IndexedDB).  
2. **Metadata** (name, size, type, folder path, favorite flag) is written to IndexedDB.  
3. **Retrieval** → metadata is read from IndexedDB; file data is fetched from OPFS/IndexedDB.  
4. **Delete** → file is moved to the trash table (soft delete); permanent delete removes both metadata and file data.

### Dependencies
- **[JSZip](https://stuk.github.io/jszip/)** – loaded from CDN for ZIP import/export.  
- All other code is vanilla JavaScript – no frameworks.

---

## 🌐 Browser Support & Performance

| Browser | OPFS | IndexedDB | Chunking | Recommended |
|---------|------|-----------|----------|-------------|
| Chrome 86+ | ✅ | ✅ | ✅ | ✅ **Best** |
| Edge 86+   | ✅ | ✅ | ✅ | ✅ **Best** |
| Firefox    | ❌ | ✅ | ✅ | ⚠️ Works (slower for large files) |
| Safari     | ❌ | ✅ | ✅ | ⚠️ Works (slower for large files) |
| Others (Opera, Brave) | ✅ (if Chromium) | ✅ | ✅ | ✅ Good |

**Performance Tips:**
- OPFS provides near‑native file I/O speeds – use a Chromium browser for the best experience.  
- For very large files (>2GB), chunking ensures reliability even in browsers without OPFS.

---

## 💾 Backup & Restore Explained

### Export ZIP
- **What it contains:** All your files (preserving folder structure) plus a `manifest.json` with account info, folders, and file metadata.  
- **How to use:** Click **Download All Files as ZIP** in Settings.  
- **Size:** The ZIP file size equals the total size of your files (plus a small metadata overhead).  

### Import ZIP
- **What it does:** Replaces all current data (files, folders, account) with the contents of the ZIP.  
- **How to use:** Click **Restore from ZIP Backup** and select your backup file.  
- **Warning:** This overwrites everything – a confirmation dialog prevents accidental loss.  

### JSON Export/Import
- **JSON Export:** Exports account credentials, recent accounts, folders, and history – **not the actual files**. Use for migrating account settings to another browser.  
- **JSON Import:** Restores the account data from a JSON backup – useful if you move to a different browser and want to keep your folder structure and favorites.

---

## 🩺 Diagnostics & Repair

### When to Use
- You see a file that won’t open (shows “data not found”).  
- You suspect some files are corrupted or missing.  
- You want to clean up orphaned metadata records.

### How to Run
1. Open **Settings** → **Diagnostics** → **Scan My Files**.  
2. The tool checks every file record against actual storage.  
3. Results show **Healthy**, **Missing**, and **Corrupted** counts.  
4. For each problematic file, you can **Remove** the record or **Repair** it by re‑uploading a replacement.

### Repair Flow
- Click **Repair** on any broken file – a file picker opens.  
- Select the replacement file – the new data overwrites the old record.  
- The file is restored and marked healthy.

---

## 🎨 Customisation

### Themes
- Choose from 20+ themes in the Settings panel.  
- Premium themes are hidden by default – toggle them on to reveal the extra options.  
- Theme selection is saved to `localStorage` – persists across sessions.

### Folder Icons
- Click the icon badge on any folder to open the icon picker.  
- Pick from 40+ emojis or upload a custom image (PNG, JPG, GIF, SVG).  
- Custom images are stored as base64 in IndexedDB – no external hosting needed.

### View & Sort Preferences
- Your chosen view mode (grid, list, etc.) and sort field/order are persisted via `localStorage`.  
- The search filter and favorites filter are session‑only.

---

## 🚀 Advanced Usage

### Large File Handling
- Files larger than 25MB are automatically chunked into smaller pieces (25MB each) when OPFS is not available.  
- This ensures reliable storage even in older browsers.  
- OPFS handles large files natively without chunking.

### Migrating to Another Browser
1. Use **JSON Export** to export account data.  
2. Use **ZIP Export** to backup all files.  
3. On the new browser, **JSON Import** first, then **ZIP Import**.  
4. Your entire cloud is restored.

### Self‑Hosting
- Because it’s a single HTML file, you can host it on any web server (or even a local network drive).  
- All storage remains local to the browser – no data is sent to the server.

### Recovery from Corruption
- If IndexedDB becomes corrupted, your browser may clear it – but OPFS files (if used) may still be present.  
- Run **Diagnostics** to scan for orphaned OPFS files and recover them.

---

## ❓ Troubleshooting & FAQ

**Q: Why can’t I upload a file larger than 2GB?**  
A: Browsers have a limit on `ArrayBuffer` size (~2GB). For very large files, use a Chromium browser with OPFS support – it handles large files more efficiently. Also, chunking helps, but the total file size is still limited by the browser’s memory.

**Q: My files disappeared!**  
A: First, check the **Trash** – you might have deleted them accidentally. If they’re not there, run **Diagnostics** to see if the records are still present. If they are, you may need to repair them.

**Q: Can I use NebulaCloud on my phone?**  
A: Yes – it’s responsive and works well on mobile browsers (Chrome/Edge on Android, Safari on iOS). However, OPFS is not supported on iOS, so performance may be slower for large files.

**Q: Is my data encrypted?**  
A: Not by default – data is stored in plain text in IndexedDB/OPFS. However, because it’s local, only you (and your browser) have access. Future versions may include optional client‑side encryption.

**Q: How do I reset my account?**  
A: You can clear all files via the **Danger Zone** in Settings, and you can remove your account by logging out and then using the **Switch Account** menu to remove the account from the list.

---

## 🤝 Contributing

We welcome contributions! Here’s how you can help:

1. **Fork** the repository.  
2. **Create a branch** (`git checkout -b feature/amazing`).  
3. **Commit** your changes (`git commit -m 'Add amazing feature'`).  
4. **Push** (`git push origin feature/amazing`).  
5. **Open a Pull Request**.

**Guidelines:**
- Keep the single‑file architecture intact – no build tools or bundlers.  
- Test thoroughly across modern browsers (Chrome, Edge, Firefox, Safari).  
- Follow the existing code style and naming conventions.  
- Update the README if you add new features.

---

## 📄 License

This project is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---

## 🗺️ Roadmap (Coming Soon)

- 🔐 **End‑to‑End Encryption** – optional file encryption for extra security.  
- 📡 **Sync Across Devices** – using WebRTC or similar peer‑to‑peer technology.  
- 🧩 **Plugin System** – extend functionality with community‑built plugins.  
- 📱 **PWA Support** – install as a standalone app on mobile and desktop.  
- 🗂️ **Shared Folders** – optional sharing with others (via local network).

---

**NebulaCloud** – privacy‑first, offline‑ready, and forever free.  
☁️ Your cloud. Your control. No compromise.

---

*Made with ❤️ and modern browser APIs.*

### Data Flow
1. **Upload** → file is read as ArrayBuffer → stored in OPFS (or chunked in IndexedDB).  
2. **Metadata** (name, size, type, folder path, favorite flag) is written to IndexedDB.  
3. **Retrieval** → metadata is read from IndexedDB; file data is fetched from OPFS/IndexedDB.  
4. **Delete** → file is moved to the trash table (soft delete); permanent delete removes both metadata and file data.

### Dependencies
- **[JSZip](https://stuk.github.io/jszip/)** – loaded from CDN for ZIP import/export.  
- All other code is vanilla JavaScript – no frameworks.

---

## 🌐 Browser Support & Performance

| Browser | OPFS | IndexedDB | Chunking | Recommended |
|---------|------|-----------|----------|-------------|
| Chrome 86+ | ✅ | ✅ | ✅ | ✅ **Best** |
| Edge 86+   | ✅ | ✅ | ✅ | ✅ **Best** |
| Firefox    | ❌ | ✅ | ✅ | ⚠️ Works (slower for large files) |
| Safari     | ❌ | ✅ | ✅ | ⚠️ Works (slower for large files) |
| Others (Opera, Brave) | ✅ (if Chromium) | ✅ | ✅ | ✅ Good |

**Performance Tips:**
- OPFS provides near‑native file I/O speeds – use a Chromium browser for the best experience.  
- For very large files (>2GB), chunking ensures reliability even in browsers without OPFS.

---

## 💾 Backup & Restore Explained

### Export ZIP
- **What it contains:** All your files (preserving folder structure) plus a `manifest.json` with account info, folders, and file metadata.  
- **How to use:** Click **Download All Files as ZIP** in Settings.  
- **Size:** The ZIP file size equals the total size of your files (plus a small metadata overhead).  

### Import ZIP
- **What it does:** Replaces all current data (files, folders, account) with the contents of the ZIP.  
- **How to use:** Click **Restore from ZIP Backup** and select your backup file.  
- **Warning:** This overwrites everything – a confirmation dialog prevents accidental loss.  

### JSON Export/Import
- **JSON Export:** Exports account credentials, recent accounts, folders, and history – **not the actual files**. Use for migrating account settings to another browser.  
- **JSON Import:** Restores the account data from a JSON backup – useful if you move to a different browser and want to keep your folder structure and favorites.

---

## 🩺 Diagnostics & Repair

### When to Use
- You see a file that won’t open (shows “data not found”).  
- You suspect some files are corrupted or missing.  
- You want to clean up orphaned metadata records.

### How to Run
1. Open **Settings** → **Diagnostics** → **Scan My Files**.  
2. The tool checks every file record against actual storage.  
3. Results show **Healthy**, **Missing**, and **Corrupted** counts.  
4. For each problematic file, you can **Remove** the record or **Repair** it by re‑uploading a replacement.

### Repair Flow
- Click **Repair** on any broken file – a file picker opens.  
- Select the replacement file – the new data overwrites the old record.  
- The file is restored and marked healthy.

---

## 🎨 Customisation

### Themes
- Choose from 20+ themes in the Settings panel.  
- Premium themes are hidden by default – toggle them on to reveal the extra options.  
- Theme selection is saved to `localStorage` – persists across sessions.

### Folder Icons
- Click the icon badge on any folder to open the icon picker.  
- Pick from 40+ emojis or upload a custom image (PNG, JPG, GIF, SVG).  
- Custom images are stored as base64 in IndexedDB – no external hosting needed.

### View & Sort Preferences
- Your chosen view mode (grid, list, etc.) and sort field/order are persisted via `localStorage`.  
- The search filter and favorites filter are session‑only.

---

## 🚀 Advanced Usage

### Large File Handling
- Files larger than 25MB are automatically chunked into smaller pieces (25MB each) when OPFS is not available.  
- This ensures reliable storage even in older browsers.  
- OPFS handles large files natively without chunking.

### Migrating to Another Browser
1. Use **JSON Export** to export account data.  
2. Use **ZIP Export** to backup all files.  
3. On the new browser, **JSON Import** first, then **ZIP Import**.  
4. Your entire cloud is restored.

### Self‑Hosting
- Because it’s a single HTML file, you can host it on any web server (or even a local network drive).  
- All storage remains local to the browser – no data is sent to the server.

### Recovery from Corruption
- If IndexedDB becomes corrupted, your browser may clear it – but OPFS files (if used) may still be present.  
- Run **Diagnostics** to scan for orphaned OPFS files and recover them.

---

## ❓ Troubleshooting & FAQ

**Q: Why can’t I upload a file larger than 2GB?**  
A: Browsers have a limit on `ArrayBuffer` size (~2GB). For very large files, use a Chromium browser with OPFS support – it handles large files more efficiently. Also, chunking helps, but the total file size is still limited by the browser’s memory.

**Q: My files disappeared!**  
A: First, check the **Trash** – you might have deleted them accidentally. If they’re not there, run **Diagnostics** to see if the records are still present. If they are, you may need to repair them.

**Q: Can I use NebulaCloud on my phone?**  
A: Yes – it’s responsive and works well on mobile browsers (Chrome/Edge on Android, Safari on iOS). However, OPFS is not supported on iOS, so performance may be slower for large files.

**Q: Is my data encrypted?**  
A: Not by default – data is stored in plain text in IndexedDB/OPFS. However, because it’s local, only you (and your browser) have access. Future versions may include optional client‑side encryption.

**Q: How do I reset my account?**  
A: You can clear all files via the **Danger Zone** in Settings, and you can remove your account by logging out and then using the **Switch Account** menu to remove the account from the list.

---

## 🤝 Contributing

We welcome contributions! Here’s how you can help:

1. **Fork** the repository.  
2. **Create a branch** (`git checkout -b feature/amazing`).  
3. **Commit** your changes (`git commit -m 'Add amazing feature'`).  
4. **Push** (`git push origin feature/amazing`).  
5. **Open a Pull Request**.

**Guidelines:**
- Keep the single‑file architecture intact – no build tools or bundlers.  
- Test thoroughly across modern browsers (Chrome, Edge, Firefox, Safari).  
- Follow the existing code style and naming conventions.  
- Update the README if you add new features.

---

## 📄 License

This project is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---

## 🗺️ Roadmap (Coming Soon)

- 🔐 **End‑to‑End Encryption** – optional file encryption for extra security.  
- 📡 **Sync Across Devices** – using WebRTC or similar peer‑to‑peer technology.  
- 🧩 **Plugin System** – extend functionality with community‑built plugins.  
- 📱 **PWA Support** – install as a standalone app on mobile and desktop.  
- 🗂️ **Shared Folders** – optional sharing with others (via local network).

---

**NebulaCloud** – privacy‑first, offline‑ready, and forever free.  
☁️ Your cloud. Your control. No compromise.

---

*Made with ❤️ and modern browser APIs.*
