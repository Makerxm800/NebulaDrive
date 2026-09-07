# ☁️ NebulaCloud

### Your private cloud — always with you, no servers, no subscriptions.

One HTML file. Open it in your browser and you've got a full-featured personal cloud. Everything stays on your device — nothing is ever sent anywhere. Built with vanilla JavaScript, OPFS, and IndexedDB. Made by **makerxm800**.

---

## 🌟 What is it?

A personal cloud that runs entirely in your browser. Upload, organise, play, and back up your files — all stored locally. No sign-ups, no subscriptions, no strings attached.

- 🔒 **100% local** — no tracking, no analytics, no server calls
- ⚡ **OPFS + IndexedDB** — fast binary storage with automatic chunking fallback
- 🎬 **Built-in media player** — audio & video with PiP, speed control, trimming, and premium animations
- 🎨 **25 themes** — 6 standard + 19 premium, all apply instantly
- 💾 **Backup & restore** — real `.zip` archives with progress tracking and cancel support

---

## 📁 File Management

Upload anything by dragging files onto the page or clicking the upload area — a progress bar tracks everything in real time.

- 📂 **Folders** — create nested folders, navigate with the clickable breadcrumb path
- ✏️ **Rename** — rename any file or folder from its action menu
- 📤 **Move** — move files anywhere with a searchable folder picker
- 🗑️ **Delete** — soft-delete into Trash, restore or permanently remove later
- ⭐ **Favourites** — star anything for quick access from the dedicated tab
- 🔍 **Search** — real-time search across everything with full paths shown
- 📦 **Batch ops** — select multiple items, invert, deselect, or delete in bulk
- 🎯 **Folder & file icons** — pick from emojis, standard SVG icons, or upload your own image
- 🏷️ **File type detection** — automatic icon for PDF, Word, Excel, PowerPoint, audio, video, code, fonts, and more

---

## 👁️ View Modes

Eight layouts, all with smooth transitions — pick what fits:

🔲 Small Grid · 📐 Medium Grid · 🖼️ Large Grid · 🎞️ Gallery · 📋 List · 📄 Compact · 🧱 Masonry · 🕐 Timeline

Sort by **name**, **size**, **date**, **type**, or **extension** — ascending or descending. Group by any field.

---

## 🎵 Media Player

### Controls

Play / pause, drag-to-seek, volume with percentage, skip ±5 seconds with overlay buttons, and playback speed from 0.5× to 3×. Three repeat modes: off, all, or one — with visual indicators.

### ✨ Visuals

- 🎶 **Spinning vinyl disc** with glow pulse animation
- 📊 **Equalizer bars** — four bouncing bars while playing
- 🌈 **Ambient glow** — animated radial drift behind the artwork
- ⏸️ **Pause/play overlay** — glass-morphism circle with morphing icon animation when toggling
- ⏩ **Video skip overlay** — back/forward 5s buttons with ripple + bounce animation on hover
- 🎵 **Track fade-in** — smooth transition when the next track loads

### ⚡ Speed Selector

Premium staggered entrance animation with active pulse glow. All speeds preserved: 0.5×, 0.75×, 1×, 1.25×, 1.5×, 1.75×, 2×, 2.5×, 3×.

### Modes

- 🔈 **Mini player** — slim bar pinned to the bottom; play, skip, volume, and repeat while you browse
- 🖥️ **Immersive fullscreen** — hides all UI after 3 seconds idle, mouse or tap brings it back
- 🎥 **Cinema mode** — true native fullscreen via the browser API
- 🖼️ **Picture-in-Picture** — pop video into a floating always-on-top window (Chrome/Edge)

### More

- ⬇️ **Download** the current track
- ✂️ **Cut video/audio** — trim with start/end sliders, real-time preview, progress bar, cancel support
- 🔽 **Minimize** to the mini player

### 📊 Quality

Click the info pill to see resolution, bitrate, file size, format, and duration. YouTube badge appears for video files — opens directly on YouTube.

---

## 📄 File Viewer

Click any file to preview it — each format gets its own viewer:

🖼️ **Images** → full-screen lightbox · 📝 **Text / code / markdown** → formatted block · 📊 **CSV** → styled table · 📑 **PDF** → embedded iframe · 🎵 **Audio / Video** → media player

---

## 🎨 Themes

Six standard themes and nineteen premium ones — toggle the switch in Settings to unlock the full collection.

Every theme has custom gradients, adaptive text colours, accent glows, and speed selector overrides. Switch instantly with no reload. Your choice is saved and persists across refreshes.

---

## 💾 Backup & Restore

- 📦 **ZIP backup** — real `.zip` archive of all your files with folder structure preserved. Progress tracking with cancel button. Available from Settings.
- 📄 **JSON export** — account data only (credentials, folders, history). Handy for migrating between browsers.
- 🔄 **Restore** — upload a backup to fully recover everything. Confirmation required — it overwrites current data.

**Moving browsers?** Export JSON first, then ZIP. Import both on the new machine and your cloud comes back.

---

## 🩺 Diagnostics

Open **Settings → Diagnostics** to scan every file record against actual storage.

- ✅ Reports healthy, missing, and corrupted files
- 🔧 **Repair** — re-upload a replacement for any broken file
- 🧹 **Remove** — clean up orphaned records in one click
- 📊 **Storage stats** — folders, files, total used, video & image counts

---

## 🚀 Getting Started

1. 📥 Download `index.html` and open it in a browser — nothing to install
2. 👤 Create an account or tap **Continue as Guest**
3. 📤 Drag files in or click the upload area
4. 📂 Make folders, sort, search, star favourites
5. 🎬 Click any file to preview or play it
6. ⚙️ Open Settings for themes, storage stats, and diagnostics

---

## ⌨️ Keyboard Shortcuts

| Key | Action |
|---|---|
| `Space` | ▶️ Play / Pause |
| `←` / `→` | ⏮️ / ⏭️ Previous / Next track |
| `F` | 🖥️ Toggle immersive fullscreen |
| `Escape` | ✕ Exit fullscreen, minimise player, or close viewer |

---

## 🌐 Browser Support

| Browser | OPFS | Experience |
|---|---|---|
| 🟢 Chrome / Edge 86+ | ✅ | ⭐ Best — near-native speeds |
| 🟢 Opera / Brave | ✅ | 👍 Good (Chromium-based) |
| 🟡 Firefox | ❌ | ⚠️ Works, slower on large files |
| 🟡 Safari | ❌ | ⚠️ Works, no OPFS on iOS |

---

## ⚡ Advanced

- 🧩 **Large files** — automatically chunked when OPFS isn't available
- 🌐 **Self-hosting** — it's one HTML file; drop it on any web server
- 🔄 **Recovery** — run Diagnostics to find orphaned OPFS files after an IndexedDB clear

---

## ❓ FAQ

**Can't upload past 2 GB?**
Browser memory limits. Use Chromium with OPFS for best results.

**Files disappeared?**
Check 🗑️ Trash first, then run 🩺 Diagnostics.

**Works on mobile?**
Yes — responsive across Android and iOS. Slower for large files on iOS (no OPFS).

**Is my data encrypted?**
Not by default — it's local, so only you and your browser can access it. Client-side encryption is on the roadmap.

---

## 🤝 Contributing

1. 🍴 Fork → 🌿 Branch → 💬 Commit → 🔀 PR
2. Keep the single-file architecture — no build tools
3. Test across Chrome, Edge, Firefox, and Safari
4. Update this README for new features

---

## 📄 License

MIT

---

## 🗺️ Roadmap

- 🔐 End-to-end encryption
- 📡 Cross-device sync via WebRTC
- 🧩 Plugin system
- 📱 PWA / installable as an app
- 🗂️ Shared folders over local network

---

☁️ **NebulaCloud** — your cloud. Your control. No compromise.
