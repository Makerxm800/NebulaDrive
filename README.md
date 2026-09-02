# ☁️ NebulaCloud

### Your private cloud — always with you, no servers, no subscriptions.

One HTML file. Open it in a browser and you have a fully-featured personal cloud. Upload files, organise them into folders, play media, and back everything up — all running locally, nothing ever leaves your device. Built with vanilla JavaScript, OPFS, and IndexedDB. Made by **makerxm800**.

---

## Highlights

- **100% local** — every file, folder, and account stays on your device. No tracking, no analytics, no server calls.
- **OPFS + IndexedDB** — near-native file speeds on Chromium, with automatic chunking fallback for large files on other browsers.
- **Media player** — audio and video with Picture-in-Picture, speed control, repeat modes, a spinning disc, and audio trimming via the Web Audio API.
- **25 themes** — 6 standard and 19 premium, all switch instantly with no reload.
- **8 view layouts** — grid, gallery, list, compact, masonry, timeline, and more.
- **Backup & restore** — real `.zip` archives with optional SHA-256 password protection.

---

## File Management

| Feature | What it does |
|---|---|
| **Upload** | Drag files onto the page or click the upload area. Real-time progress bar. Global drop overlay lets you drop anywhere. |
| **Download** | One click saves any file to your device. |
| **Folders** | Create nested folders with the **+ New Folder** button. Navigate with the clickable breadcrumb path bar. |
| **Rename** | Rename files or folders from action buttons or the **More** menu. |
| **Move** | Move files and folders to any destination — a searchable modal shows your folder tree. |
| **Delete** | Soft-delete into **Trash**. Restore or permanently remove from there. Empty trash in one click. |
| **Favorites** | Star any file or folder. Dedicated **Favorites** tab shows all starred items across every folder. A filter button in the toolbar shows only favourites in the current view. |
| **Search** | Debounced real-time search across all files and folders, with full path in results. |
| **Batch operations** | Toggle selection mode, select multiple items, invert the selection, deselect, or delete them all at once. |
| **Folder icons** | Click a folder's icon badge to pick from a set of emojis or upload your own image (PNG, JPG, GIF, SVG). |

---

## View Modes

Eight layouts to choose from, all with smooth transitions:

| Mode | Style |
|---|---|
| **Small Grid** | Compact cards with thumbnails |
| **Medium Grid** | Default balanced layout |
| **Large Grid** | Bigger cards, more breathing room |
| **Gallery** | Image-focused, large previews |
| **List** | Table-style rows |
| **Compact** | Minimal, maximum density |
| **Masonry** | Pinterest-style uneven layout |
| **Timeline** | Sorted by date with visual time markers |

Sorting by **name**, **size**, **date**, **type**, or **extension** — ascending or descending.

---

## Media Player

### Playback

- **Play / Pause**, **Previous**, **Next**, **Skip back 5s**, **Skip forward 5s**
- **Drag-to-seek** progress bar
- **Volume** with percentage display and mouse wheel support
- **Speed control** — 0.5×, 0.75×, 1×, 1.25×, 1.5×, 1.75×, 2×
- **Repeat modes** — Off, All, One, with clear visual indicators
- **Queue-based playback** — all playable files in the current folder queue up automatically

### Visuals

- **Spinning vinyl disc** with glow pulse animation
- **Audio equalizer** — four bouncing bars while playing
- **Ambient glow** — animated radial drift behind artwork
- **Pause overlay** — animated pop-in/out icon
- **Track fade-in** — smooth transition when the next track loads

### Modes

- **Mini player** — slim bar pinned to the bottom of the screen. Play/pause, skip, volume, seek, repeat, expand to full player, or close.
- **Immersive fullscreen** — custom overlay that hides all UI after 3 seconds idle. Mouse or tap brings controls back.
- **Cinema mode** — true native fullscreen via the browser API.
- **Picture-in-Picture** — pop video out into a floating always-on-top window (Chrome/Edge).

### More menu

- **Download** the currently playing file
- **Cut / Trim** — trim audio with a start/end slider. Downloads the trimmed clip via Web Audio API (`OfflineAudioContext`).
- **Minimize** to the mini player
- **Picture-in-Picture**

### Quality

- Click the gear icon to see **resolution**, **bitrate**, **file size**, **format**, and **duration** (for audio/video)
- A YouTube badge appears for files named like `[VIDEO_ID].mp4` — opens the video on YouTube

---

## File Viewer

Clicking a file opens it in a purpose-built viewer:

| File type | Viewer |
|---|---|
| **Images** | Full-screen lightbox preview |
| **Text, code, markdown, YAML, XML** | Syntax-formatted `<pre>` block |
| **CSV** | Rendered as a styled HTML table (up to 500 rows) |
| **PDF** | Embedded in an iframe |
| **Audio / Video** | Opens in the media player |

---

## Themes

All themes apply instantly — no reload.

### Standard

| Theme | Style |
|---|---|
| **Dark Nebula** | Deep purple-blue gradients, the default |
| **Light Aurora** | Soft grey-blue light mode |
| **Azure Abyss** | Teal and deep blue |
| **Crimson Horizon** | Warm reds fading to dark |
| **Nordic Frost** | Cool Nordic blue-grey |
| **Dracula Noir** | Classic Dracula-inspired palette |

### Premium

Toggle the **Premium Themes** switch in Settings to reveal the full collection.

| Theme | Style |
|---|---|
| **Midnight Gold** | Dark base with warm gold accents |
| **Velvet Rose** | Deep burgundy-rose gradients |
| **Emerald Noir** | Rich greens on a dark canvas |
| **Platinum Frost** | Icy blues and metallic highlights |
| **Crimson Ember** | Deep reds with ember glow |
| **Sapphire Royale** | Royal blue and sapphire tones |
| **Obsidian Chrome** | Near-black with chrome-grey accents |
| **Aurora Borealis** | Northern-lights-inspired greens and purples |
| **Cosmic Latte** | Warm, muted cosmic pastels |
| **Forest Whisper** | Natural greens and soft earth tones |
| **Rose Gold** | Soft rose-metallic gradients |
| **Nebula Rose** | Purple-rose nebula tones |
| **Arctic Midnight** | Cold midnight blues and whites |
| **Sunset Boulevard** | Warm sunset oranges and reds |
| **Twilight Amethyst** | Purple-tinted twilight tones |
| **Carbon Fiber** | Dark, textured carbon fibre look |
| **Emerald Frost** | Green-tinted frosty highlights |
| **Crimson Velvet** | Deep crimson velvet richness |
| **Solarized Dusk** | Solarized palette, dusk variant |

Each theme has custom gradient backgrounds, adaptive text colours, accent glows, and matching UI controls.

---

## Backup & Restore

### ZIP backup (all files)

- Creates a real `.zip` archive with **all your files** preserving folder structure, plus a `manifest.json` with users, folders, and metadata.
- Optionally set a **password** — a SHA-256 hash is stored in `META.json` inside the archive.
- Restoring replaces everything — a confirmation dialog prevents accidental loss.
- Uses [JSZip](https://cdnjs.cloudflare.com/ajax/libs/jszip/3.10.1/jszip.min.js) loaded from CDN.

### JSON export (account data only)

- Exports credentials, recent accounts, folders, and history.
- Useful for moving account settings between browsers without transferring actual files.
- Does **not** include file data.

### Migration workflow

1. Export JSON (account data)
2. Export ZIP (all files)
3. On the new browser: import JSON, then import ZIP

---

## Diagnostics & Repair

Open **Settings → Diagnostics → Scan My Files**.

- Checks every file record against actual browser storage
- Reports **Healthy**, **Missing**, and **Corrupted** files with a progress bar
- **Repair** — re-upload a replacement for any broken file
- **Remove broken records** — clean up orphaned metadata in one click
- **Live storage stats** — folders, files, total storage, video count, image count, and disk usage (when available)

---

## Getting Started

1. **Download** `index.html` and open it in your browser. No server, no installation.
2. **Create an account** or tap **Continue as Guest** for instant access.
3. **Upload** by dragging files in or clicking the upload area.
4. **Organise** — create folders, star favourites, sort and search.
5. **Play** — click any image, audio, video, text, CSV, or PDF to open it.
6. **Customise** — open Settings to pick a theme, run diagnostics, view storage stats, or manage your account.

---

## Keyboard Shortcuts

| Key | When | Action |
|---|---|---|
| `Space` | Media player open | Play / Pause |
| `→` | Media player open | Next track |
| `←` | Media player open | Previous track |
| `F` | Media player open | Toggle immersive fullscreen |
| `Escape` | Media player open | Exit fullscreen or minimise to mini player |
| `Escape` | File viewer open | Close the viewer |

---

## How It Works

```
┌──────────────────────────────────────────────┐
│ NebulaCloud                                  │
│                                              │
│  🗄️  IndexedDB  — metadata                   │
│     accounts · folders · history · trash      │
│     file metadata · folder icons (base64)    │
│                                              │
│  💾  OPFS       — file content               │
│     fast, secure binary storage              │
│     (fallback: IndexedDB chunked storage)    │
│                                              │
│  📦  JSZip      — backup & restore           │
│     ZIP export/import with optional          │
│     SHA-256 password hash                    │
│                                              │
│  🎵  Web Audio  — audio trimming             │
│     OfflineAudioContext for cut/trim         │
└──────────────────────────────────────────────┘
```

**Upload** — file is read as an ArrayBuffer, stored in OPFS (or chunked into IndexedDB when OPFS isn't available). Metadata goes into IndexedDB.

**Retrieval** — metadata is read from IndexedDB, file data is fetched from OPFS or reconstructed from chunks.

**Delete** — soft-delete moves the record to the trash table. Permanent delete clears both metadata and file data.

---

## Browser Support

| Browser | OPFS | Experience |
|---|---|---|
| Chrome 86+ | ✅ | ⭐ Best — near-native speeds |
| Edge 86+ | ✅ | ⭐ Best |
| Opera, Brave (Chromium) | ✅ | Good |
| Firefox | ❌ | Works — slower on large files |
| Safari | ❌ | Works — slower on large files, no OPFS on iOS |

OPFS gives you the fastest storage. Use a Chromium browser for the best experience with large files.

---

## Customisation

- **Themes** — 25 total. Premium ones are hidden by default behind a toggle in Settings. Saved to `localStorage`.
- **Folder icons** — emoji picker or your own image, stored locally as base64 in IndexedDB.
- **View and sort preferences** — persist across sessions via `localStorage`.

---

## Advanced Usage

### Large files

Files over ~25 MB are automatically chunked when OPFS isn't available. This keeps things reliable on Firefox and Safari. Chromium handles large files natively via OPFS.

### Self-hosting

It's a single HTML file. Drop it on any web server, local network drive, or serve it however you like. All storage stays local to the browser — the server never sees any data.

### Recovery

If IndexedDB ever gets cleared by the browser, OPFS files may still exist on disk. Run **Diagnostics** to scan for orphaned files and reattach them.

### IndexedDB versioning

The database (`NebulaCloudDB`) is currently at version 19. The schema includes stores for `folders`, `files`, `fileChunks`, `fileData`, `history`, `trash`, `folderIcons`, and `customIcons`.

---

## Troubleshooting & FAQ

**Can't upload files past 2 GB?**
Browsers cap `ArrayBuffer` around 2 GB. Use a Chromium browser with OPFS support for the best experience with large files.

**Files disappeared?**
Check **Trash** first — you may have deleted them accidentally. Not there? Open **Diagnostics** to see if records still exist and repair them.

**Works on mobile?**
Yes — responsive across Android Chrome/Edge and iOS Safari. OPFS isn't supported on iOS, so large files will be slower.

**Is my data encrypted?**
Not by default — it's stored locally in plain form. Only you and your browser can reach it. Optional client-side encryption is on the roadmap.

**How do I reset my account?**
Clear all files via the **Danger Zone** in Settings. Remove accounts from the **Switch Account** menu.

**How much storage can I use?**
It depends on the browser. Chrome and Edge typically allow about 50% of disk space, Firefox around 10%, and Safari about 50 GB per domain. Check your browser's DevTools under Storage for exact numbers.

**Can I share files with others?**
Not directly — export a ZIP and send it. Peer-to-peer sharing is planned.

---

## Contributing

If you want to help improve NebulaCloud:

1. Fork the repository
2. Create a branch (`git checkout -b feature/something`)
3. Commit your changes
4. Push and open a pull request

**Rules of this project:**

- Keep the single-file architecture — no build tools, no bundlers
- Test across modern browsers (Chrome, Edge, Firefox, Safari)
- Follow the existing code style
- Update this README if you add features

---

## License

MIT — see the [LICENSE](LICENSE) file.

---

## Roadmap

- 🔐 **End-to-end encryption** — optional, client-side
- 📡 **Cross-device sync** — WebRTC peer-to-peer
- 🧩 **Plugin system**
- 📱 **PWA support** — install as a standalone app
- 🗂️ **Shared folders** — local network sharing

---

**NebulaCloud** — offline-ready, private, and free.

☁️ Your cloud. Your control.
