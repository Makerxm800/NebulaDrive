# ☁️ NebulaDrive

### Your private cloud — always with you, no servers, no subscriptions.

One HTML file. Open it in your browser and you've got a full-featured personal cloud. Everything stays on your device — nothing is ever sent anywhere. Built with vanilla JavaScript, OPFS, and IndexedDB. Made by **makerxm800**.

---

## 🌟 What is it?

A personal cloud that runs entirely in your browser. Upload, organise, play, and back up your files — all stored locally. No sign-ups, no subscriptions, no strings attached.

- 🔒 **100% local** — no tracking, no analytics, no server calls
- ⚡ **OPFS + IndexedDB** — fast binary storage with automatic chunking fallback
- 🎬 **Built-in media player** — audio & video with PiP, speed control, trimming, precise seeking, and premium animations
- 🎨 **30 themes + Liquid Glass** — 11 standard + 19 premium, plus a site-wide glass effect toggle
- 💾 **Backup & restore** — real `.zip` archives with progress tracking and cancel support
- ✨ **Premium UI everywhere** — spring animations, ripple presses, staggered entrances, glow accents
- 🌐 **8 languages** — globe button opens a centered menu (new visitors get a welcome picker first): nav, toolbar, menus, dialogs, cards, profile, themes, player, settings, hover hints, empty states — everything switches instantly and remembers your choice

---

## 📁 File Management

Upload anything by dragging files onto the page or clicking the upload area — a progress bar tracks everything in real time.

- 🧭 **Breadcrumb** — Root home pill + glowing active-folder pill, chevron separators, back/forward/refresh, staggered slide-in
- 📂 **Folders** — create nested folders, navigate with the clickable breadcrumb path
- ✏️ **Rename** — rename any file or folder from its card menu via an in-app dialog (no native popups)
- 📁 **New folders** — created through the same premium dialog, autofocused with the name preselected
- 📤 **Move** — move files anywhere with a searchable folder picker
- ⭐ **Favourites** — star anything; the tab icon turns gold when active, card stars fill yellow
- 🔍 **Search** — real-time search across everything with full paths shown
- 📦 **Batch ops** — Select mode with a sticky command bar: Select All, Invert, Deselect, bulk **Download**, bulk Delete (with a real confirm dialog). Trash and History have matching select bars with Restore/Delete.
- 🎯 **Folder & file icons** — pick from emojis, standard SVG icons, or upload your own image
- 🏷️ **File type detection** — automatic icon for PDF, Word, Excel, PowerPoint, audio, video, code, fonts, and more
- 🩹 **Missing MIME fix** — MP3/WAV/FLAC/M4A/OGG and videos whose browser reported no type are auto-detected from the extension, so they always open in the player

### ⋯ Card menus

Every card has a **More** menu — a real menu, not three similar buttons:

- 📝 **Header** — shows whether it's file or folder options plus the item name
- 📖 **Descriptions** — each action explains itself ("Move to another folder", "Move file to Trash"…)
- 🎨 **Distinct actions** — Rename (cyan), Move (amber), Delete (rose), Cut (violet), Repair (orange)
- ✨ **Animation** — spring pop with cascading item entrance
- 📍 **Smart positioning** — spans the card so it never clips, flips upward near the viewport bottom, works in all 4 view modes and on touch devices

### 🔘 File buttons vs folders

- **Files** — blue **View** + green **Get** duo plus a dashed More row
- **Folders** — star + solid More
- Labels always fit: ellipsis truncation in every view mode, icon-first buttons that never squash

---

## 🗑️ Trash

A proper trash system with zero native browser popups — every delete flows through a premium in-app confirm dialog (with native-confirm fallback so deletes never break).

- 🧾 **Header** — Trash title, live item count badge, Empty Trash (red) + Restore All (green) buttons that disable when empty
- ✅ **Select** — tick checkboxes on cards, then **Restore selected** or **Delete selected** in the bulk bar
- 🎞️ **Animations** — cards shrink out on restore/delete, cascade in on load
- 🔄 **Restore** — single restore, Restore All (cascades out first), or bulk restore

---

## 🕘 History

- 🔎 **Search + filters** — All, Uploaded, Deleted, Restored, Downloaded pills plus live search
- 👆 **Click to jump** — any entry with a file jumps straight to it with a flash highlight
- ✅ **Select mode** — toggle Select, tick entries, bulk-delete with confirm (files stay untouched)
- 🧹 **Clear** — premium confirm; empty and no-match states included
- 🏷️ **True action icons** — upload arrow, download arrow, trash, restore… each in its type colour

---

## ⭐ Favourites

Gold-starred header with a live count. Everything you've starred, from anywhere — click to jump to where it lives. Empty state guides you to tap the star.

---

## 👤 Profile

Click your avatar for a centered menu with backdrop blur and spring entrance.

- 🖼️ **Banner that works** — upload any image, auto-downscaled and saved per account
- 🎨 **Profile themes shortcut** — jumps straight to Settings themes
- ⚠️ **Danger Zone lives here now** — collapsed by default; Clear Cache, Delete Account, Terminate, all with warning styling and real confirm dialogs (removed from Settings)
- 📱 **iPhone-safe overlays** — blur effects auto-disable on touch devices where they blank scrolling cards
- 🟢 **Statuses** — Online, Organizing, Uploading, Backing up, Away, Offline, or your own custom text — all translated

---

## 👁️ View Modes

Four file-management layouts with smooth transitions — pick what fits:

🔲 Small Grid · 📐 Medium Grid · 🖼️ Large Grid · 📋 List

Sort by **name**, **size**, **date**, **type**, or **extension** — ascending or descending. Group by any field.

- 📱 **Responsive toolbar** — PC keeps the full row; tablet drops search to its own row; mobile scrolls Sort/View/Favorites in one row with full-width dropdowns
- 📜 **Scrollframe menus** — Sort and View menus scroll inside the viewport on small screens, submenus drop below instead of off-screen
- ⭐ **Favourites has its own layout** — tidy adaptive grid on PC, compact two-column cards on mobile (row layouts stay in Files)
- 🔘 **Premium toolbar pills** — Sort and View have ripple, press squash, and an active glow while open; only one menu opens at a time

---

## 🎵 Media Player

### Controls

Play / pause, drag-to-seek, volume with percentage, skip ±5 seconds with overlay buttons, and playback speed from 0.5× to 3×. Three repeat modes: off, all, or one — with visual indicators.

### 🎯 Progress & precise seeking (PC beta)

- The bar thickens with an accent ring + glow on hover; the thumb grows
- Hover shows a YouTube-style glass tooltip with a live frame thumbnail + timestamp
- Pull **up** while dragging for fine 0.22× seeking with a highlighted precise state
- Hover preview and precise mode are PC-only — touch devices just drag to seek

### ✨ Visuals

- 🎶 **Spinning vinyl disc** with glow pulse animation
- 📊 **Equalizer bars** — four bouncing bars while playing
- 🌈 **Ambient glow** — animated radial drift behind the artwork
- ⏸️ **Pause/play overlay** — glass-morphism circle with morphing icon animation when toggling
- ⏩ **Video skip overlay** — back/forward 5s buttons with ripple + bounce animation on hover
- 🎵 **Track fade-in** — smooth transition when the next track loads

### ⚙️ Settings menu

- ⚡ **Playback speed + Repeat hero rows** — accent-tinted with white value pills, press-squash with pill pop
- 🎛️ **Remade toggles** — Ambient mode, Volume boost, Stable Volume, Captions, Annotations: bordered spring switches with shine knobs and accent glow when on
- Long titles truncate cleanly; controls wrap on narrow screens; hover lifts disabled on touch

### Modes

- 🔈 **Mini player** — slim bar pinned to the bottom; play, skip, volume, and repeat while you browse
- 🖥️ **Fullscreen (Nebula premium)** — the Fullscreen button opens the premium overlay (top bar with file title + NebulaCloud badge + auto-hiding glass "press Esc" hint, accent progress bar, floating glass dock: Prev / Play-Pause / Next / Mute with state-synced SVG icons, volume slider + % (mirrors the normal player control), time pill, "In this file ›" shortcut to Media Info (opens inside fullscreen, no need to exit), Settings gear, Exit, accent ••• More) and takes over the **full device screen** via the browser Fullscreen API. Gear / More open the REAL settings and more menus inside fullscreen. Follows the active theme, hides UI after 3s idle.
- 🎥 **Cinema mode (in-page immersive, improved)** — same premium dock but stays in the page (no device fullscreen), for browsing-friendly watching
- 🎥 **Cinema mode** — true native fullscreen via the browser API (separate from the Nebula Fullscreen above)
- 🖼️ **Picture-in-Picture** — pop video into a floating always-on-top window (Chrome/Edge)
- 📱 **Mobile music fix** — smaller artwork + compact info card on phones; fullscreen dock wraps, Esc hint and chapter pill hide on small screens
- ⌨️ **Keyboard button hidden on touch/iPhone** — the keyboard-shortcuts icon never shows on coarse-pointer or iOS devices (shortcuts stay available on desktop)

## ℹ️ Info Button — About NebulaCloud

New **Info** button in the top navbar (next to Settings). Toggles a premium About GUI with everything about NebulaCloud: files, player, fullscreen, themes, safety, shortcuts — plus quick actions (Take the Tour, Open Settings, Try Fullscreen Player). Shortcuts: press `I` to toggle, `Esc` to close, or click outside the card.

### More

- ⬇️ **Download** the current track
- ✂️ **Cut video/audio** — trim with start/end sliders, real-time preview, progress bar, cancel support
- 🔽 **Minimize** to the mini player

### 📊 Quality

Click the info pill to see resolution, bitrate, file size, format, and duration. YouTube badge appears for video files — opens directly on YouTube.

---

## 📄 File Viewer

Click any file to preview it — each format gets its own viewer:

🖼️ **Images** → full-screen lightbox (pinch-zoom + swipe through photos on mobile, spring entrance, safe-area aware) · 📝 **Text / code / markdown** → formatted block · 📊 **CSV** → styled table · 📑 **PDF** → embedded iframe · 🎵 **Audio / Video** → media player

---

## 🎨 Themes + Liquid Glass

Eleven standard themes and nineteen premium ones — toggle the switches in **Settings → Themes** to show each collection. The active theme always shows a check; switching never drops Liquid Glass or Reduce Motion.

- ⚡ **Instant switching** — custom gradients, adaptive text colours, accent glows, and speed selector overrides. No reload; your choice persists.
- 📱 **iPhone-friendly Light theme** — same colours, minus the costly extras: no glow layers, no shine sweeps, no backdrop blur, no floating orbs. Flat and still, easy on mobile GPUs.
- 🫧 **Liquid Glass ✦** — site-wide glass effect toggle at the top of the Themes section. Deep blur + saturation on bars, cards, and dialogs. Works with every theme, saved across sessions.
- 🐢 **Reduce Motion ✦** — site-wide calm mode next to it (also honors your OS setting). Kills animations and transitions everywhere.
- ✨ **No shine sweeps** — all gloss-shimmer loops removed on every device for lag-free scrolling.
- 🔤 **Readable accents** — pale-accent themes (Midnight Gold, Pine Grove, Graphite) get dark text on pills and buttons instead of washed-out white.
- 🎚️ **Remade switches** — spring sliding knobs that widen when pressed, glow when on, keyboard accessible.

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
2. 🧭 Hit the **Tour** button next to Select for an 11-step guided walkthrough (fully translated, works on mobile)
3. 👤 Create an account or tap **Continue as Guest**
4. 📤 Drag files in or click the upload area
5. 📂 Make folders, sort, search, star favourites
6. 🎬 Click any file to preview or play it
7. ⚙️ Open Settings for Liquid Glass, themes, storage stats, and diagnostics

---

## ⌨️ Keyboard Shortcuts

| Key | Action |
|---|---|
| `Space` / `K` | ▶️ Play / Pause |
| `←` / `→` or `J` / `L` | ⏪ / ⏩ Seek 5s (Shift: prev / next track) |
| `↑` / `↓` | 🔊 Volume |
| `M` | 🔇 Mute |
| `F` | 🖥️ Toggle Nebula premium fullscreen (audio + video) |
| `I` | ℹ️ Toggle About NebulaCloud info panel |
| `P` | 🖼️ Picture-in-Picture |
| `,` / `.` | ⚡ Speed down / up |
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

**An MP3 or video won't play?**
File types are auto-detected from the extension, so it should just work. If it still fails, run 🩺 Diagnostics and use 🔧 Repair to re-upload it.

**Works on mobile?**
Yes — responsive across Android and iOS, touch-safe menus and controls. Music keeps playing with the screen locked (lock-screen controls included); keep Volume Boost off for background listening, as iOS suspends boosted audio on lock but resumes it when you return. Hover-only extras (seek preview, precise mode) are PC-only. Large files are slower on iOS (no OPFS).

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
