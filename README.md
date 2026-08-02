<div align="center">

# ☁️ NebulaCloud

### Your unlimited, self‑hosted personal cloud — in a single HTML file.

[![Status](https://img.shields.io/badge/status-beta-8b7cf6?style=for-the-badge)](#)
[![License](https://img.shields.io/badge/license-MIT-667eea?style=for-the-badge)](#-license)
[![Platform](https://img.shields.io/badge/platform-browser-764ba2?style=for-the-badge)](#)
[![No Server](https://img.shields.io/badge/server-none%20required-06b6d4?style=for-the-badge)](#)

No installs. No accounts on someone else's server. No subscription.
Just open the file and your cloud is *right there*.

</div>

---

## ✨ Why NebulaCloud

Most "cloud storage" means trusting a company with your files and paying monthly for the privilege. NebulaCloud flips that: **everything lives in your own browser**, under your control, for free — while still feeling like a premium, modern cloud app.

<div align="center">

| 🔒 Private | 💸 Free | ⚡ Fast | 🎨 Beautiful |
|:---:|:---:|:---:|:---:|
| Nothing leaves your browser | No subscriptions, ever | OPFS‑backed storage engine | Glassmorphism UI, 11 themes |

</div>

---

## 📋 Table of Contents

- [Features](#-features)
- [Quick Start](#-quick-start)
- [Themes](#-themes)
- [Media Player](#-media-player)
- [Technical Details](#%EF%B8%8F-technical-details)
- [Customisation](#-customisation)
- [License](#-license)
- [Contributing](#-contributing)

---

## ✨ Features

<table>
<tr><td width="50%" valign="top">

**📂 Files & Folders**
- Unlimited storage via OPFS, with automatic IndexedDB fallback
- Nested folders with a live breadcrumb trail
- Recursive folder sizes (subfolders included)
- Rename, move, and delete — for both files *and* folders
- Global search across your entire cloud, with path results
- Drag & drop anywhere on the page, not just one box
- 6 view modes: Small / Medium / Large Grid, Gallery, List, Compact

</td><td width="50%" valign="top">

**🎬 Media & Previews**
- Premium media player with animated vinyl disc, equalizer, fullscreen video
- Docked mini‑player bar — keep browsing while music plays
- Repeat modes, real Prev/Next queue, one‑tap download
- In‑app preview for PDF, TXT, CSV, JSON, Markdown, and more
- Image lightbox for quick previews

</td></tr>
<tr><td width="50%" valign="top">

**👤 Accounts**
- Sign up, sign in with Google, or continue as Guest
- **Switch Account** — jump between recent sessions instantly
- Full action history: uploads, downloads, moves, renames

</td><td width="50%" valign="top">

**🎨 Design**
- 11 themes across two tiers — Classic and ✨ Premium
- Glassmorphism, smooth animations, tactile button feedback
- Fully responsive — proper iPhone support (safe areas, fullscreen video, touch targets)
- Settings panel doubles as a live storage dashboard

</td></tr>
</table>

---

## 🚀 Quick Start

```
1. Download index.html
2. Double-click to open it in your browser — nothing to install
3. Create an account (or tap "Continue as Guest")
4. Drop in some files and start organizing
```

That's it. Your files stay in *this* browser, on *this* device — refreshing the page even drops you back into the exact folder you were in.

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

Every theme has its own accent color, not just a different background — buttons, progress bars, and highlights shift with it. Switch anytime from **Settings ⚙️**.

---

## 🎧 Media Player

A docked, Spotify‑style mini‑player keeps your music going while you keep browsing — expand it back to the full player anytime, with repeat modes, a real queue built from your current folder, and a fullscreen button for video that works correctly on both desktop *and* iPhone.

---

## 🛠️ Technical Details

| | |
|---|---|
| **Storage engine** | OPFS (Origin Private File System) — fast direct binary I/O |
| **Fallback** | Chunked IndexedDB, automatic on unsupported browsers |
| **Metadata** | IndexedDB, separate from file data for fast folder browsing |
| **Compatibility** | Chrome, Edge, Firefox, Safari (modern versions) |
| **Requirement** | Secure context (HTTPS or localhost) for OPFS; degrades gracefully otherwise |

---

## 🔧 Customisation

- **Themes** — Settings ⚙️ → pick from 11 built‑in themes
- **View mode** — toolbar → 6 layouts to browse your way
- **Google Sign‑In** — drop your own Client ID into the script to enable it on your domain

---

## 📝 License

MIT — free to use, modify, and distribute.

---

## 🤝 Contributing

Found a bug or have an idea? Open an issue or submit a pull request.

<div align="center">

**Made with ☁️ by the NebulaCloud team.**

</div>
