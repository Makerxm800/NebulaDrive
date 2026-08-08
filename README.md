<div align="center">

# ☁️ NebulaCloud

### Your private, offline‑first personal cloud — in a single HTML file.

[![Status](https://img.shields.io/badge/status-beta-8b7cf6?style=for-the-badge)](#)
[![License](https://img.shields.io/badge/license-MIT-667eea?style=for-the-badge)](#license)
[![Platform](https://img.shields.io/badge/platform-browser-764ba2?style=for-the-badge)](#)
[![No Server](https://img.shields.io/badge/server-none%20required-06b6d4?style=for-the-badge)](#)

**No installs, no accounts on someone else's server, no subscription.**  
Just open the file and your cloud is *right there* – private, fast, and beautiful.

</div>

---

## ✨ Why NebulaCloud

Most "cloud storage" means trusting a company with your files and paying monthly for the privilege. NebulaCloud flips that: **everything lives in your own browser**, under your control, for free — while still feeling like a premium, modern cloud app.

| 🔒 Private | 💸 Free | ⚡ Fast | 🎨 Beautiful |
|:---:|:---:|:---:|:---:|
| Nothing leaves your browser | No subscriptions, ever | OPFS‑backed storage engine | Glassmorphism UI, 11 themes |

---

## 📋 Table of Contents

- [Features](#-features)
- [Quick Start](#-quick-start)
- [File Management](#-file-management)
- [Media Player](#-media-player)
- [Themes](#-themes)
- [Technical Details](#%EF%B8%8F-technical-details)
- [Customisation](#-customisation)
- [License](#-license)
- [Contributing](#-contributing)

---

## ✨ Features

### 📂 Files & Folders
- **Unlimited storage** via OPFS (Origin Private File System), with automatic IndexedDB fallback for unsupported browsers.
- **Nested folders** with a live breadcrumb trail – click any part to jump back instantly.
- **Recursive folder sizes** – see the total size of everything inside a folder.
- **Rename, move, and delete** – both files *and* folders, with everything safely moved to the Trash.
- **Global search** across your entire cloud, showing results with their full path.
- **Drag & drop** anywhere on the page – not just a single zone.
- **6 view modes**: Small Grid, Medium Grid, Large Grid, Gallery, List, Compact – switch instantly from the toolbar.
- **Select multiple items** with checkboxes, then batch‑delete or move them.

### 🎬 Media & Previews
- **Premium media player** with an animated vinyl disc, live equalizer, and fullscreen video support (works on desktop and mobile).
- **Docked mini‑player bar** – keep browsing while your music or podcast keeps playing.
- **Repeat modes**: Off, All, One – with visual feedback.
- **Real queue** – Next/Previous buttons cycle through all playable files in the current folder.
- **In‑app preview** for PDF, TXT, CSV, JSON, Markdown, and more – no external viewers needed.
- **Image lightbox** – click any image to preview it full‑screen.

### 👤 Accounts & Sessions
- **Sign up**, **sign in**, or **continue as Guest** – no email required.
- **Switch Account** – jump between recent sessions instantly, preserving each user's data.
- **Action history** – every upload, download, move, and rename is logged with timestamps.
- **Rename your account** – change your display name without losing data.

### 🛡️ Backup & Diagnostics
- **Full ZIP backup** – export all your files (preserving folder structure) plus a `manifest.json` with metadata. The backup is a standard ZIP file you can open with any archive tool.
- **Import ZIP backup** – restore everything in one click; the app reloads automatically when done.
- **Storage diagnostics** – scan every file record against actual storage to find missing or corrupted files, and remove broken entries with a single button.

### 🎨 Design & UI
- **11 themes** across Classic and Premium tiers – each with its own accent color, not just a background change.
- **Glassmorphism** – subtle transparency, blur, and shadows for a modern, premium feel.
- **Smooth animations** – responsive feedback on every button, hover, and transition.
- **Fully responsive** – proper support for iPhones (safe‑area padding, fullscreen video that rotates correctly, touch‑friendly targets).
- **Settings dashboard** – live storage overview showing folders, files, used space, video/image counts, and disk usage.

---

## 🚀 Quick Start

1. **Download** `index.html`.
2. **Double‑click** to open it in your browser – nothing to install.
3. **Create an account** (or tap "Continue as Guest").
4. **Drop in some files** and start organising.

That’s it. Your files stay in *this* browser, on *this* device – refreshing the page even drops you back into the exact folder you were in.

> ⚠️ **Note**: For the best experience (especially with large files), open the page over **HTTPS** or **localhost**. OPFS (Origin Private File System) requires a secure context, but IndexedDB fallback works everywhere.

---

## 🎨 Themes

<div align="center">

| Classic | Classic | Classic |
|---|---|---|
| 🌙 Dark Nebula | ☀️ Light Aurora | 🌊 Deep Ocean |
| 🌅 Sunset Glow | ❄️ Nord | 🧛 Dracula |
| 🌤️ Solarized | | |

| ✨ Premium | ✨ Premium |
|---|---|
| 🖤 Midnight Gold | 🌹 Velvet Rose |
| 💎 Emerald Noir | 🤍 Platinum Frost |

</div>

Every theme uses its own accent colour – buttons, progress bars, and highlights shift to match. Switch anytime from **Settings ⚙️**.

---

## 🎧 Media Player

A docked, Spotify‑style mini‑player keeps your music going while you keep browsing – expand it back to the full player anytime, with repeat modes, a real queue built from your current folder, and a fullscreen button for video that works correctly on both desktop *and* iPhone.

**Keyboard shortcuts** (when the player is focused):

| Key | Action |
|-----|--------|
| `Space` | Play / Pause |
| `→` | Next track |
| `←` | Previous track |
| `F` | Toggle fullscreen (video only) |
| `Escape` | Exit fullscreen / minimize |

---

## 🛠️ Technical Details

| | |
|---|---|
| **Storage engine** | OPFS (Origin Private File System) – fast, direct binary I/O for large files. |
| **Fallback** | Chunked IndexedDB (5 MB chunks) – automatic when OPFS is unavailable. |
| **Metadata** | IndexedDB – separate from file data for fast folder browsing and sorting. |
| **File chunking** | Files are split into 5 MB pieces for reliable storage of multi‑gigabyte files. |
| **Compatibility** | Chrome 90+, Edge 90+, Firefox 110+, Safari 15+ (OPFS support varies; fallback covers all). |
| **Security** | Everything runs locally; no data is sent anywhere. For OPFS, a secure context (HTTPS or localhost) is required. |

---

## 🔧 Customisation

- **Themes** – Settings ⚙️ → pick from 11 built‑in themes.
- **View mode** – toolbar → 6 layouts to browse your way.
- **Google Sign‑In** – drop your own Client ID into the script to enable it on your domain.
- **Liquid Glass** – toggle a more pronounced glass effect on/off (Settings → Interface).

---

## 📝 License

MIT — free to use, modify, and distribute.

---

## 🤝 Contributing

Found a bug or have an idea? Open an issue or submit a pull request.

<div align="center">

**Made with ☁️ by the NebulaCloud team.**

</div>
