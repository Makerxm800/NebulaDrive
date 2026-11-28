'use strict';
const { app, BrowserWindow, Menu, protocol, shell, session } = require('electron');
const path = require('node:path');
const fs = require('node:fs');
const os = require('node:os');

const SMOKE = process.argv.includes('--smoke');
const APP_DIR = path.join(__dirname, 'app');
const ORIGIN = 'nebula://app';

// A fresh, throw-away profile for the automated check so it never touches real data.
if (SMOKE) app.setPath('userData', fs.mkdtempSync(path.join(os.tmpdir(), 'nebula-smoke-')));

// Serve the app from a secure private scheme (not file://) so browser storage,
// IndexedDB and the private file system behave exactly like on the website.
protocol.registerSchemesAsPrivileged([
  { scheme: 'nebula', privileges: { standard: true, secure: true, supportFetchAPI: true, stream: true, corsEnabled: true } },
]);

const MIME = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8',
  '.json': 'application/json', '.webmanifest': 'application/manifest+json', '.png': 'image/png',
  '.svg': 'image/svg+xml', '.ico': 'image/x-icon', '.woff2': 'font/woff2', '.txt': 'text/plain; charset=utf-8',
};

let win = null;

if (!SMOKE && !app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on('second-instance', () => { if (win) { if (win.isMinimized()) win.restore(); win.focus(); } });
}

function buildMenu() {
  const isMac = process.platform === 'darwin';
  const template = [
    ...(isMac ? [{ label: app.name, submenu: [{ role: 'about' }, { type: 'separator' }, { role: 'hide' }, { role: 'hideOthers' }, { role: 'unhide' }, { type: 'separator' }, { role: 'quit' }] }] : []),
    { label: 'Edit', submenu: [{ role: 'undo' }, { role: 'redo' }, { type: 'separator' }, { role: 'cut' }, { role: 'copy' }, { role: 'paste' }, { role: 'selectAll' }] },
    { label: 'View', submenu: [{ role: 'reload' }, { role: 'forceReload' }, { type: 'separator' }, { role: 'resetZoom' }, { role: 'zoomIn' }, { role: 'zoomOut' }, { type: 'separator' }, { role: 'togglefullscreen' }, { role: 'toggleDevTools' }] },
    { label: 'Window', submenu: [{ role: 'minimize' }, { role: 'close' }] },
  ];
  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
}

function createWindow() {
  win = new BrowserWindow({
    width: 1400, height: 900, minWidth: 480, minHeight: 520,
    show: false, backgroundColor: '#0a0a14', title: 'NebulaDrive', autoHideMenuBar: true,
    icon: path.join(__dirname, 'build', 'icon.png'),
    webPreferences: { contextIsolation: true, nodeIntegration: false, sandbox: true, spellcheck: false },
  });
  win.once('ready-to-show', () => { if (!SMOKE) win.show(); });
  win.webContents.setWindowOpenHandler(({ url }) => {
    if (/^https?:/i.test(url)) { shell.openExternal(url); return { action: 'deny' }; }
    if (/^(blob:|nebula:)/i.test(url)) return { action: 'allow' };
    return { action: 'deny' };
  });
  win.webContents.on('will-navigate', (event, url) => {
    if (!url.startsWith(ORIGIN)) { event.preventDefault(); if (/^https?:/i.test(url)) shell.openExternal(url); }
  });
  win.loadURL(ORIGIN + '/index.html');
  win.on('closed', () => { win = null; });
  return win;
}

// Automated check used by the build: opens the real app, signs in as guest,
// clicks every tab and confirms storage works. Never runs for normal users.
async function runSmoke(w) {
  const errors = [];
  w.webContents.on('console-message', (...a) => {
    const lvl = typeof a[1] === 'number' ? a[1] : a[0].level;
    const msg = typeof a[1] === 'number' ? a[2] : a[0].message;
    if (lvl === 3 || lvl === 'error') errors.push(String(msg).slice(0, 300));
  });
  const result = { ok: false, platform: process.platform, electron: process.versions.electron };
  try {
    await new Promise((res, rej) => {
      w.webContents.once('did-finish-load', res);
      w.webContents.once('did-fail-load', (e, c, d) => rej(new Error('load failed: ' + d)));
    });
    const page = await w.webContents.executeJavaScript(`(async () => {
      const sleep = ms => new Promise(r => setTimeout(r, ms));
      const waitFor = async (fn, ms = 25000) => { const t = Date.now(); while (Date.now() - t < ms) { try { if (fn()) return true; } catch (e) {} await sleep(150); } return false; };
      const r = { proto: location.protocol, secure: isSecureContext, jszip: typeof JSZip === 'function' };
      try { r.opfs = !!(await navigator.storage.getDirectory()); } catch (e) { r.opfs = false; r.opfsErr = String(e); }
      await waitFor(() => document.getElementById('guestBtn') || (document.getElementById('mainApp') && getComputedStyle(document.getElementById('mainApp')).display !== 'none'));
      const g = document.getElementById('guestBtn'); if (g && g.offsetParent !== null) g.click();
      r.mainShown = await waitFor(() => { const m = document.getElementById('mainApp'); return m && getComputedStyle(m).display !== 'none'; });
      r.tabs = document.querySelectorAll('.nav-tab').length;
      if (r.mainShown) {
        for (const n of ['favorites', 'history', 'trash', 'files']) { document.querySelector('.nav-tab[data-tab="' + n + '"]').click(); await sleep(250); }
        r.activeAfter = document.querySelector('.nav-tab.active').dataset.tab;
      }
      r.idb = await new Promise(res => { const q = indexedDB.open('NebulaCloudDB'); q.onsuccess = () => { q.result.close(); res(true); }; q.onerror = () => res(false); });
      r.title = document.title;
      return r;
    })()`);
    Object.assign(result, page);
    result.jsErrors = errors.filter(e => /Uncaught|TypeError|ReferenceError|SyntaxError/.test(e));
    result.otherConsoleErrors = errors.length - result.jsErrors.length;
    result.ok = page.proto === 'nebula:' && page.secure && page.jszip && page.opfs && page.mainShown &&
      page.tabs === 4 && page.activeAfter === 'files' && page.idb && result.jsErrors.length === 0;
  } catch (e) { result.error = String((e && e.message) || e); }
  try { fs.writeFileSync(process.env.SMOKE_OUT || path.join(os.tmpdir(), 'nebula-smoke.json'), JSON.stringify(result)); } catch (e) {}
  console.log('SMOKE ' + JSON.stringify(result));
  app.exit(result.ok ? 0 : 1);
}

app.whenReady().then(() => {
  protocol.handle('nebula', async (request) => {
    try {
      const url = new URL(request.url);
      let rel = decodeURIComponent(url.pathname);
      if (rel === '/' || rel === '') rel = '/index.html';
      const file = path.normalize(path.join(APP_DIR, rel));
      if (file !== APP_DIR && !file.startsWith(APP_DIR + path.sep)) return new Response('Forbidden', { status: 403 });
      const data = await fs.promises.readFile(file);
      return new Response(data, { headers: { 'Content-Type': MIME[path.extname(file).toLowerCase()] || 'application/octet-stream' } });
    } catch (e) { return new Response('Not found', { status: 404 }); }
  });
  const allowed = new Set(['fullscreen', 'clipboard-read', 'clipboard-sanitized-write', 'media', 'mediaKeySystem', 'notifications', 'persistent-storage']);
  session.defaultSession.setPermissionRequestHandler((wc, permission, cb) => cb(allowed.has(permission)));
  session.defaultSession.setPermissionCheckHandler((wc, permission) => allowed.has(permission));
  buildMenu();
  const w = createWindow();
  if (SMOKE) {
    setTimeout(() => { console.log('SMOKE timeout'); app.exit(2); }, 90000);
    runSmoke(w);
  }
});

app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0 && app.isReady()) createWindow(); });
