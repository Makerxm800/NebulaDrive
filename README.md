# ☁️ NebulaCloud – Unlimited Personal Cloud Storage (Beta)

**Your unlimited, self‑hosted cloud storage – all in a single HTML file.**

> **⚠️ BETA** – This is a preview release. Features are being refined. Feedback is welcome!

---

## ✨ Features

- **Unlimited storage** – powered by the **Origin Private File System (OPFS)** for fast, direct binary reads/writes, with automatic fallback to IndexedDB on browsers that don't support it yet. Metadata and file data are kept separate so browsing folders stays fast no matter how much you've uploaded.
- **Folders & navigation** – create, open, and move files between folders with a breadcrumb trail. Folder cards show a live, recursive total size (including everything inside subfolders).
- **All file types** – images, videos, audio, documents, executables, scripts, archives – everything is supported, with no restrictive upload categories.
- **Drop files anywhere** – drag and drop works across the *entire page*, not just a small box, with a full‑screen visual cue while dragging. Click‑to‑browse works too.
- **6 view modes** – Small / Medium / Large Grid, Gallery (extra‑large thumbnails), List, and Compact (dense list) — switch instantly from the toolbar.
- **Sort & organize** – sort by Name, Size, or Date, ascending or descending, from a polished dropdown.
- **Media player** – premium built‑in player for video and audio:
  - Animated vinyl‑style disc with glow and equalizer for audio
  - Fullscreen support and properly fitted video playback
  - Real Prev/Next queue built from the files in your current folder
  - Repeat modes (off / all / one), volume, and a "more" menu with quick download
  - A **docked mini‑player bar** (Spotify/Windows‑style) that keeps playing when you minimize the full player, so you can keep browsing while your music plays
- **File viewer** – preview PDFs, TXT, CSV, JSON, Markdown, logs, and more directly in the app, no forced download. CSVs render as a real table.
- **Image lightbox** – quick, clean image previews.
- **User accounts** – create an account, sign in with Google, or continue as a guest.
- **History** – tracks uploads, downloads, moves, and folder creations.
- **11 themes**, including brand‑new **Premium** options:
  - *Classic:* Dark Nebula, Light Aurora, Deep Ocean, Sunset Glow, Nord, Dracula, Solarized — each with richer gradients and its own accent color
  - *✨ Premium:* Midnight Gold, Velvet Rose, Emerald Noir, Platinum Frost
- **Premium UI** – glassmorphism, smooth animations, an animated "New Folder" button, and a fully responsive layout.

---

## 🚀 Quick Start

1. **Download** `index.html` (or copy the code).
2. **Double‑click** to open in your browser – no server or installation needed.
3. **Create an account** (or use Guest) and start uploading files.
4. **Organise** your files with folders.
5. **Preview** images, PDFs, text/CSV files, and play videos and music directly in the app.

---

## 📸 Screenshots

*(The interface is clean, modern, and fully functional.)*

---

## 🔧 Customisation

- **Themes** – change from the Settings panel (⚙️). 11 themes to choose from, including 4 Premium options.
- **View mode** – pick your preferred layout from the toolbar; your choice is remembered per session.
- **Google Sign‑In** – replace the client ID in the script to enable it on your domain.

---

## 🛠️ Technical Details

- **Storage**: OPFS (Origin Private File System) as the primary backend for file binary data, with automatic fallback to chunked IndexedDB storage on unsupported browsers. Metadata always lives in IndexedDB for fast queries.
- **Performance**: File data is only read from disk when you actually open/play/download it. Folder sizes are computed recursively on load.
- **Compatibility**: Works on all modern browsers (Chrome, Firefox, Edge, Safari). OPFS requires a secure context (HTTPS or localhost); the app degrades gracefully to IndexedDB otherwise.

---

## 📝 License

MIT – free to use, modify, and distribute.

---

## 🤝 Contributing

Found a bug or have a suggestion? Open an issue or submit a pull request.

---

**Made with ☁️ by the NebulaCloud team.**
