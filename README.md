# 🌌 NebulaCloud

> Your private, offline‑first cloud – in a single HTML file.

[![Version](https://img.shields.io/badge/version-BETA_2.0-blue)](#)
[![License](https://img.shields.io/badge/license-MIT-green)](#)
[![Platform](https://img.shields.io/badge/platform-browser-lightgrey)](#)
[![Server](https://img.shields.io/badge/server-none%20required-brightgreen)](#)
[![Storage](https://img.shields.io/badge/storage-OPFS%20%2B%20IndexedDB-orange)](#)

---

## 📖 Overview

**NebulaCloud** is a fully‑featured personal cloud that runs entirely in your browser.  
No sign‑ups, no subscriptions, no third‑party servers – just you, your files, and a premium interface that rivals paid cloud services.

| 🔒 Privacy | ⚡ Performance | 🎨 Design | 🎬 Media |
|------------|---------------|-----------|----------|
| 100% local storage | OPFS + IndexedDB | Glass‑morphism UI | Built‑in media player |
| No data ever leaves your device | Fast binary storage | 20+ premium themes | Audio & video support |
| No tracking, no analytics | Chunked large files | Responsive & smooth | Picture‑in‑Picture |

---

## ✨ Core Features

### ☁️ File Management

| Feature | Description |
|---------|-------------|
| **Upload & Download** | Drag‑and‑drop support, click to browse, progress tracking |
| **Folder Creation** | Organise your files with nested folders |
| **Rename, Move, Delete** | Full file/folder lifecycle management |
| **⭐ Favorites** | Star any item for quick access (dedicated Favorites tab) |
| **🗑️ Trash** | Deleted items go to trash; restore or permanently delete |
| **🔍 Search** | Instant filtering across all files and folders |
| **Multiple Views** | Grid (small/medium/large), Gallery, List, Compact |
| **Sorting** | By name, size, or date (ascending/descending) |
| **Batch Operations** | Select multiple, invert selection, bulk delete (moves to trash) |
| **Folder Icons** | Choose from 40+ emojis or upload custom images (PNG, JPG, SVG) |

### 🎬 Premium Media Player

A fully‑featured player for audio and video files:

| Feature | Description |
|---------|-------------|
| **Playback Controls** | Play/Pause, Seek, Volume, Speed (0.5× – 2×), ±5s Skip |
| **Repeat Modes** | Off, All, One – with visual state indicator |
| **Quality Dashboard** | Gear icon shows resolution, bitrate (kbps), file size, format, duration |
| **Mini Player** | Stays at the bottom of the screen while you browse |
| **Picture‑in‑Picture** | Video only – watch in a floating window |
| **Immersive Fullscreen** | Hides UI for a cinema‑like experience |
| **True Fullscreen** | Native fullscreen (Cinema Mode) |
| **YouTube Integration** | Auto‑detects `[VIDEO_ID].mp4` and provides a direct YouTube link |

### 💎 Premium Themes

Over **20 carefully crafted themes** to match your mood:

| 🌙 Standard Themes | ⭐ Premium Themes |
|-------------------|------------------|
| Dark Nebula | Midnight Gold ✦ |
| Light Aurora | Velvet Rose ✦ |
| Azure Abyss | Emerald Noir ✦ |
| Crimson Horizon | Platinum Frost ✦ |
| Nordic Frost | Crimson Ember ✦ |
| Dracula Noir | Sapphire Royale ✦ |
| Solarized Dusk | Obsidian Chrome ✦ |
| | Aurora Borealis ✦ |
| | Cosmic Latte ✦ |
| | Forest Whisper ✦ |
| | Rose Gold ✦ |
| | Nebula Rose ✦ |

> 💡 Premium themes can be toggled on/off via the Settings panel.

### 🗄️ Backup & Restore

| Feature | Description |
|---------|-------------|
| **📦 Export ZIP** | Download all files as a real `.zip` – preserves folder structure and metadata |
| **📥 Import ZIP** | Full recovery of account, files, and folders from a backup |
| **📄 JSON Export** | Account data, users, folders, and history – for switching devices |
| **📥 JSON Import** | Restore account credentials and data from a JSON backup |

### 🩺 Diagnostics & Repair

- **🔍 File Integrity Scan** – checks every file against actual browser storage  
- **🔧 Repair Individual Files** – re‑upload a replacement to fix corruption  
- **🧹 Remove Broken Records** – clean up dangling metadata entries  
- **📊 Live Storage Stats** – view folder/file counts, usage, video/image counts

### 🛡️ Security & Privacy

| Aspect | Implementation |
|--------|----------------|
| **Data Storage** | 100% local – no data ever sent to any server |
| **Authentication** | Username + password (stored locally in IndexedDB) |
| **Guest Mode** | Instant access without creating an account |
| **File Storage** | OPFS (Origin Private File System) – secure, performant |
| **Fallback** | IndexedDB when OPFS is unavailable |
| **Chunking** | Files >25MB split into reliable chunks |

---

## 🏁 Getting Started

### 1. Open the File
Simply open `index.html` in your browser – no installation, no build tools, no server.

### 2. Create an Account
Enter a username and password – everything stays local.

> 💡 Prefer instant access? Click **Continue as Guest** – no account needed.

### 3. Start Uploading
- **Drag & Drop** – drag files anywhere on the page  
- **Click to Browse** – use the file picker via the upload area

### 4. Organise
- Create folders with the **+ New Folder** button  
- Star favourites with the ⭐ icon  
- Switch views and sorting from the toolbar

### 5. Preview & Play
- Click any file to preview it – images, audio, video, text, PDF, CSV  
- Use the built‑in media player for audio and video  
- Click the gear icon for quality details

### 6. Customise
- Open **Settings** (⚙️ gear icon)  
- Choose a theme – or enable Premium Themes  
- View storage statistics and run diagnostics

---

## ⌨️ Keyboard Shortcuts

| Shortcut | Context | Action |
|----------|---------|--------|
| `Space` | Media Player active | Play / Pause |
| `➡️ Arrow Right` | Media Player active | Next track |
| `⬅️ Arrow Left` | Media Player active | Previous track |
| `F` | Media Player active | Toggle immersive fullscreen |
| `Escape` | Media Player active | Close fullscreen / Minimize player |

---

## 🛠️ Technical Architecture

### 🗂️ Storage Layers

```
┌─────────────────────────────────────────┐
│              NebulaCloud                │
├─────────────────────────────────────────┤
│  ┌───────────────────────────────────┐ │
│  │     🗄️ IndexedDB (Metadata)      │ │
│  │  Users · Folders · History · Trash│ │
│  │  File metadata · Folder icons     │ │
│  └───────────────────────────────────┘ │
│  ┌───────────────────────────────────┐ │
│  │     💾 OPFS (File Content)        │ │
│  │  Fast, secure binary storage       │ │
│  │  (fallback: IndexedDB chunks)      │ │
│  └───────────────────────────────────┘ │
│  ┌───────────────────────────────────┐ │
│  │     📦 JSZip (Backup/Restore)     │ │
│  │  ZIP export/import functionality   │ │
│  └───────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

### 📦 Dependencies
- **[JSZip](https://stuk.github.io/jszip/)** – loaded from CDN for ZIP operations

### 🌐 Browser Support

| Browser | OPFS | IndexedDB | Recommended |
|---------|------|-----------|-------------|
| Chrome 86+ | ✅ | ✅ | ✅ Best |
| Edge 86+   | ✅ | ✅ | ✅ Best |
| Firefox    | ❌ | ✅ | ⚠️ Works (slower) |
| Safari     | ❌ | ✅ | ⚠️ Works (slower) |

---

## 🧪 Running Locally

```bash
# Clone the repository
git clone https://github.com/yourusername/nebula-cloud.git

# Navigate into the directory
cd nebula-cloud

# Open in your browser (any modern browser works)
open index.html
```

> 🔥 **No server required** – just open the file.

---

## 🚀 Advanced Usage

### Custom Icon Upload
When changing a folder icon, you can upload custom images (PNG, JPG, GIF, SVG).  
The image is stored directly in IndexedDB – no external hosting required.

### Folder Icons
Folder icons persist across browser sessions – the icon picker remembers your choices.

### Large File Support
Files larger than 25MB are automatically chunked for reliable storage when OPFS is unavailable.

### Recovery
If your account data is lost, the built‑in **Diagnostics** tool can scan for and recover orphaned file records.

---

## 🤝 Contributing

We welcome contributions! Here's how:

1. **Fork** the repository  
2. **Create a branch** (`git checkout -b feature/amazing`)  
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)  
4. **Push** (`git push origin feature/amazing`)  
5. **Open a Pull Request**

**Guidelines:**
- Keep the single‑file architecture intact  
- Test thoroughly across modern browsers  
- Follow the existing code style and naming conventions  
- Document new features in the README

---

## 📄 License

This project is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---

## 💬 Acknowledgments

- [JSZip](https://stuk.github.io/jszip/) for ZIP operations  
- Built with ❤️ and modern browser APIs (IndexedDB, OPFS, File API)

---

**NebulaCloud** – privacy‑first, offline‑ready, and forever free.  
☁️ Your cloud. Your control. No compromise.
