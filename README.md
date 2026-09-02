# ☁️ NebulaCloud

### Your private cloud — always with you, no servers, no subscriptions.

One HTML file. Open it in a browser and you have a fully-featured personal cloud. Nothing ever leaves your device. Built with vanilla JavaScript, OPFS, and IndexedDB. Made by **makerxm800**.

---

## What is it?

A personal cloud that runs entirely in your browser. Upload files, organise them into folders, play media, and back everything up — all stored locally with IndexedDB and OPFS. No sign-ups, no subscriptions, no hidden costs.

- **100% local** — no tracking, no analytics, no server calls
- **OPFS + IndexedDB** — fast binary storage with automatic chunking fallback
- **Media player** — audio and video with PiP, speed control, and trimming
- **25 themes** — 6 standard, 19 premium, all apply instantly
- **Backup** — real `.zip` archives with optional password protection

---

## File Management

Upload via drag-and-drop or click. Create nested folders, rename, move, and delete. Soft-delete goes to Trash first. Star favourites for quick access. Search across everything in real time.

Batch operations let you select multiple items at once. Choose from 8 view layouts (grid, gallery, list, compact, masonry, timeline) and sort by name, size, date, or type. Folder icons are customisable with emojis or your own image.

---

## Media Player

Play, pause, seek, skip ±5s, volume, speed (0.5×–2×), and repeat modes. A spinning disc with glow animation and bouncing equalizer bars while playing.

- **Mini player** — stays pinned to the bottom when you close the full player
- **Immersive fullscreen** — auto-hiding controls after 3 seconds idle
- **Cinema mode** — true native fullscreen
- **Picture-in-Picture** — floating video window (Chrome/Edge)
- **Trim audio** — cut clips with a slider, downloads via Web Audio API

---

## File Viewer

Click any file to open it — images in a lightbox, text/code/markdown as formatted blocks, CSV as a styled table, PDF in an iframe, and audio/video in the media player.

---

## Themes

6 standard and 19 premium (toggle the switch in Settings to reveal them). Every theme has custom gradients, adaptive text colours, and accent glows — all switch instantly with no reload.

---

## Backup & Restore

- **ZIP** — real `.zip` of all files (folder structure preserved) plus manifest. Optional SHA-256 password.
- **JSON** — account data only (credentials, folders, history). Useful for migrating between browsers.

---

## Diagnostics

Scan every file record against actual storage. Reports healthy, missing, and corrupted files. Repair broken files by re-uploading, or remove orphaned records in one click.

---

## Getting Started

1. Download `index.html` and open it in a browser — nothing to install
2. Create an account or tap **Continue as Guest**
3. Upload by dragging files in or clicking the upload area
4. Create folders, sort, search, star favourites
5. Click any file to preview or play it
6. Open Settings for themes, storage stats, diagnostics, and account management

---

## Keyboard Shortcuts

| Key | Action |
|---|---|
| `Space` | Play / Pause (media player) |
| `←` / `→` | Previous / Next track |
| `F` | Toggle immersive fullscreen |
| `Escape` | Exit fullscreen, minimise player, or close file viewer |

---

## Browser Support

| Browser | OPFS | Notes |
|---|---|---|
| Chrome / Edge 86+ | ✅ | Best experience |
| Opera / Brave | ✅ | Good (Chromium-based) |
| Firefox | ❌ | Works, slower on large files |
| Safari | ❌ | Works, no OPFS on iOS |

---

## Advanced

- **Large files** — automatically chunked when OPFS isn't available
- **Self-hosting** — it's one HTML file; drop it on any web server
- **Recovery** — run Diagnostics to find orphaned OPFS files after an IndexedDB clear

---

## FAQ

**Can't upload past 2 GB?** Browser memory limits. Use Chromium with OPFS for best results.

**Files disappeared?** Check Trash first, then run Diagnostics.

**Works on mobile?** Yes — responsive across Android and iOS. Slower for large files on iOS (no OPFS).

**Encrypted?** Not by default — it's local, so only you and your browser can see it. Encryption is on the roadmap.

---

## Contributing

1. Fork, branch, commit, PR
2. Keep the single-file architecture — no build tools
3. Test across Chrome, Edge, Firefox, Safari
4. Update this README for new features

---

## License

MIT — see [LICENSE](LICENSE).

---

## Roadmap

- 🔐 End-to-end encryption
- 📡 Cross-device sync via WebRTC
- 🧩 Plugin system
- 📱 PWA / installable app
- 🗂️ Shared folders over local network

---

☁️ Your cloud. Your control.
