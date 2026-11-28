# NebulaDrive Desktop

The NebulaDrive web app in its own window, for **Windows** and **macOS** (Linux is planned).
It runs the exact same code as the website, so every feature is the same: files, folders,
favorites, history, trash, backups, themes, languages and the built-in players.
Everything stays on your computer. No servers, no accounts needed, no subscriptions.

## Download

Open the [latest release](../../../releases/latest):

- **Windows:** `NebulaDrive-Setup-<version>.exe` (installer) or `NebulaDrive-<version>-portable.exe` (no install).
- **macOS:** `NebulaDrive-<version>-mac.dmg` (Intel and Apple silicon). The app is not notarized, so the first time, right-click it and choose Open.

Windows may show a blue "Windows protected your PC" screen because the app is not signed.
Choose **More info**, then **Run anyway**.

## How it is built

`scripts/prepare.js` copies the website's `index.html` into the app and bundles the zip library
so backups work offline. So when the website improves, the desktop app improves with it on the next build.
GitHub Actions builds the installers and, before releasing, opens the real app and checks that it
starts, stores data, and switches between all tabs.

## Run it yourself

```
cd desktop
npm install
npm start
```

## Your data

Stored in your user profile folder for NebulaDrive (on Windows: `%APPDATA%\NebulaDrive`).
The older command-line version in `../pc` is separate and unchanged.
