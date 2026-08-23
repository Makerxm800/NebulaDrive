# 🌌 NebulaCloud

> Your private, offline‑first personal cloud — in a single HTML file.

[![Version](https://img.shields.io/badge/version-BETA-blue)](#)
[![License](https://img.shields.io/badge/license-MIT-green)](#)
[![Platform](https://img.shields.io/badge/platform-browser-lightgrey)](#)
[![Server](https://img.shields.io/badge/server-none%20required-brightgreen)](#)

**No installs, no external accounts, no subscriptions.**  
Just open the file and your cloud is *right there* – private, fast, and beautiful.

---

## 🚀 Why NebulaCloud

Most “cloud storage” means trusting your files to a third party and paying monthly.  
NebulaCloud flips that: **everything stays in your own browser**, under your control – forever free – while feeling like a premium cloud app.

| 🌌 Private | 🚀 Free | ⚡ Fast | 🎨 Beautiful |
|------------|---------|---------|--------------|
| Nothing leaves your browser | No subscriptions, ever | OPFS‑backed storage | Glassmorphism UI + 20+ themes |

---

## ✨ Features

### ☁️ File Management
- **Upload, download, rename, move, delete** – full CRUD for files and folders  
- **Folder icons** – choose from 40+ emojis or upload your own custom image  
- **⭐ Favorites** – star files and folders for quick access  
- **🔍 Instant search** – filters your entire cloud  
- **Multiple view modes** – Grid (small/medium/large), Gallery, List, Compact  
- **Sorting** – by name, size, or date (ascending/descending)  
- **Batch selection** – select all, invert selection, bulk delete (moves to trash)

### 🎬 Premium Media Player
- **Audio & video playback** – supports common formats  
- **Full controls** – play/pause, seek, volume, speed (0.5× – 2×), ±5s skip  
- **Repeat modes** – Off, All, One  
- **Quality info** – gear icon shows resolution, bitrate, size, format, duration  
- **Mini player** – stays at the bottom while you browse  
- **Picture‑in‑Picture** (video) – watch in a floating window  
- **Immersive fullscreen** – hides UI for a cinema experience  
- **YouTube badge** – automatically links to YouTube when a video filename contains `[VIDEO_ID].mp4`

### 💎 Premium Themes
Choose from **20+ themes** to match your mood – toggle “Show Premium Themes” in Settings:

| 🌙 Standard Themes | ⭐ Premium Themes |
|-------------------|------------------|
| Dark Nebula       | Midnight Gold    |
| Light Aurora      | Velvet Rose      |
| Azure Abyss       | Emerald Noir     |
| Crimson Horizon   | Platinum Frost   |
| Nordic Frost      | Crimson Ember    |
| Dracula Noir      | Sapphire Royale  |
| Solarized Dusk    | Obsidian Chrome  |
|                   | Aurora Borealis  |
|                   | Cosmic Latte     |
|                   | Forest Whisper   |
|                   | Rose Gold        |
|                   | Nebula Rose      |

### 🗄️ Backup & Restore
- **📦 Export all files as ZIP** – preserves folder structure and metadata  
- **📥 Import from ZIP** – full recovery of account, files, and folders  
- **📄 JSON account export/import** – for switching devices or backing up credentials

### 🩺 Diagnostics & Repair
- **🔍 Scan for missing or corrupted files** – verifies each file against storage  
- **🔧 Repair individual files** – re‑upload a replacement to fix corruption  
- **🧹 Remove dangling records** – clean up broken metadata entries

### 🛡️ Security & Privacy
- **🔒 Zero data sent anywhere** – everything is local to your browser  
- **👤 Account system** – username + password (stored locally)  
- **👻 Guest mode** – instant, ephemeral use  
- **💾 OPFS (Origin Private File System)** – fast, secure file storage  
- **🗃️ IndexedDB fallback** – for browsers without OPFS

---

## 🏁 Getting Started

1. **📂 Open the HTML file** – no installation, no build tools.  
2. **👤 Create an account** or **👻 Continue as Guest**.  
3. **📤 Upload files** – drag and drop anywhere, or click the upload area.  
4. **📁 Create folders** with the **+ New Folder** button.  
5. **👁️ Preview any file** by clicking it – images, audio, video, text, PDF, CSV all work.  
6. **⭐ Toggle favorites** with the star icon.  
7. **🎨 Change themes** from the Settings panel (gear icon).  
8. **🎬 Use the media player** – click any audio/video file, then open the gear icon for quality details.

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Space` | Play / Pause (when media player is active) |
| `➡️ Arrow Right` | Next track |
| `⬅️ Arrow Left` | Previous track |
| `F` | Toggle immersive fullscreen |
| `Escape` | Close fullscreen / Minimize player |

---

## 🛠️ Technical Details

### 🗂️ Storage Architecture
- **🗄️ IndexedDB** – stores metadata, user accounts, folder structure, history, trash, and icons.  
- **💾 OPFS (Origin Private File System)** – stores the actual file content for large files (faster and more secure than IndexedDB for binary data).  
- **🧩 Chunking** – files >25MB are split into chunks (fallback when OPFS is unavailable).

### 📦 Dependencies
- **[JSZip](https://stuk.github.io/jszip/)** – used for creating and reading ZIP backups (loaded from CDN).

### 🌐 Browser Support

| Browser | OPFS Support | Fallback |
|---------|--------------|----------|
| Chrome 86+ | ✅ Full | IndexedDB |
| Edge 86+   | ✅ Full | IndexedDB |
| Firefox    | ❌       | IndexedDB |
| Safari     | ❌       | IndexedDB |

---

## 🧪 Running Locally

1. **Clone the repository** (or download the single HTML file):
   ```bash
   git clone https://github.com/yourusername/nebula-cloud.git
   cd nebula-cloud
