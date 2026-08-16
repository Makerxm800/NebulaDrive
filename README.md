# ☁️ NebulaCloud

Your private, offline-first personal cloud — in a single HTML file.

![Version](https://img.shields.io/badge/version-BETA-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-browser-lightgrey)
![Server](https://img.shields.io/badge/server-none%20required-brightgreen)

**No installs, no accounts on someone else's server, no subscription.**  
Just open the file and your cloud is *right there* – private, fast, and beautiful.

---

## Why NebulaCloud

Most "cloud storage" means trusting a company with your files and paying monthly for the privilege. NebulaCloud flips that: **everything lives in your own browser**, under your control, for free — while still feeling like a premium, modern cloud app.

| Private | Free | Fast | Beautiful |
|---------|------|------|-----------|
| Nothing leaves your browser | No subscriptions, ever | OPFS-backed storage engine | Glassmorphism UI, 20+ themes |

---

## Table of Contents

- [Why NebulaCloud](#why-nebulacloud)
- [Features](#features)
  - [File Management](#file-management)
  - [Premium Media Player](#premium-media-player)
  - [Themes](#themes)
  - [Backup & Restore](#backup--restore)
  - [Diagnostics](#diagnostics)
  - [Security & Privacy](#security--privacy)
- [Getting Started](#getting-started)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Technical Details](#technical-details)
- [Running Locally](#running-locally)
- [Contributing](#contributing)
- [License](#license)

---

## Features

### File Management

- **Upload, download, rename, move, and delete** files and folders
- **Folder icon picker** – choose from 40+ emojis or upload your own custom image
- **Favorites** – star any file or folder to quickly find it later
- **Search** – instant filtering across all files and folders
- **Multiple view modes** – Grid (small/medium/large), Gallery, List, Compact
- **Sorting** – by name, size, or date (ascending/descending)
- **Batch selection** – select all, invert selection, and bulk delete

### Premium Media Player

A fully-featured media player that works for both audio and video files, with:

- **Playback controls** – play/pause, seek, volume, playback speed (0.5× – 2×), skip ±5 seconds
- **Repeat modes** – Off, All, One
- **Quality info** (⚙️) – view resolution, bitrate (kbps), file size, format, and duration for any audio or video file
- **Mini player** – stays at the bottom of the screen while you continue browsing
- **Picture-in-Picture** – watch videos in a floating window (video only)
- **Fullscreen** – immersive mode (hides UI) and native fullscreen
- **YouTube badge** – automatically detects YouTube video IDs in filenames (e.g. `[VIDEO_ID].mp4`) and provides a direct link

### Themes

Choose from **20+ premium themes** to match your mood:

| Standard Themes | Premium Themes ⭐ |
|-----------------|------------------|
| Dark Nebula | Midnight Gold |
| Light Aurora | Velvet Rose |
| Azure Abyss | Emerald Noir |
| Crimson Horizon | Platinum Frost |
| Nordic Frost | Crimson Ember |
| Dracula Noir | Sapphire Royale |
| Solarized Dusk | Obsidian Chrome |
| | Aurora Borealis |
| | Cosmic Latte |
| | Forest Whisper |
| | Rose Gold |
| | Nebula Rose |

### Backup & Restore

- **Download all files as a ZIP** – preserves folder structure and file metadata
- **Restore from ZIP backup** – full recovery of your account, files, and folders
- **JSON account export/import** – for switching between devices or backing up user credentials

### Diagnostics

- **Scan for missing or corrupted files** – verifies every file against your browser's storage
- **Repair individual files** – re‑upload a replacement file to fix corruption
- **Remove dangling records** – clean up broken metadata entries

### Security & Privacy

- **Zero data sent to any server** – all storage is local to your browser
- **Account system** with username and password (stored locally)
- **Guest mode** for instant, ephemeral use
- **OPFS (Origin Private File System)** for secure, performant file storage
- **IndexedDB fallback** for browsers without OPFS support

---

## Getting Started

1. **Open the HTML file** – no installation, no build tools.
2. **Create an account** or **continue as Guest**.
3. **Upload files** – drag and drop anywhere on the page, or click the upload area.
4. **Create folders** with the **+ New Folder** button.
5. **Preview any file** by clicking it – images, audio, video, text, PDFs, CSVs all work.
6. **Toggle favorites** with the star icon on any file or folder.
7. **Change themes** from the Settings panel (gear icon).
8. **Use the media player** – click any audio/video file, then open the gear icon to see playback quality details.

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Space` | Play / Pause (when media player is active) |
| `Arrow Right` | Next track |
| `Arrow Left` | Previous track |
| `F` | Toggle immersive fullscreen |
| `Escape` | Close fullscreen / Minimize player |

---

## Technical Details

### Storage Architecture

- **IndexedDB** – stores metadata, user accounts, folder structure, history, trash, and icons.
- **OPFS (Origin Private File System)** – stores the actual file content for large files. It's faster and more secure than IndexedDB for binary data.
- **Chunking** – files larger than 25MB are split into chunks for reliable storage (fallback only when OPFS is unavailable).

### Dependencies

- **[JSZip](https://stuk.github.io/jszip/)** – used for creating and reading ZIP backups (loaded from CDN).

### Browser Support

| Browser | OPFS Support | Fallback |
|---------|--------------|----------|
| Chrome 86+ | ✅ Full | IndexedDB |
| Edge 86+ | ✅ Full | IndexedDB |
| Firefox | ❌ | IndexedDB |
| Safari | ❌ | IndexedDB |

---

## Running Locally

1. **Clone the repository:**
   ```bash
   git clone https://github.com/yourusername/nebula-cloud.git
   cd nebula-cloud
