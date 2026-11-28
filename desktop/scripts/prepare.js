'use strict';
// Copies the web app (../index.html) into desktop/app so the desktop app always
// runs exactly the same code as the website, and bundles the zip library locally
// so backups work offline.
const fs = require('node:fs');
const path = require('node:path');

const root = path.join(__dirname, '..', '..');
const desktop = path.join(__dirname, '..');
const out = path.join(desktop, 'app');

fs.rmSync(out, { recursive: true, force: true });
fs.mkdirSync(path.join(out, 'vendor'), { recursive: true });
fs.mkdirSync(path.join(desktop, 'build'), { recursive: true });

let html = fs.readFileSync(path.join(root, 'index.html'), 'utf8');
const cdn = /<script src="https:\/\/cdnjs\.cloudflare\.com\/ajax\/libs\/jszip\/[^"]*jszip\.min\.js">/;
if (!cdn.test(html)) throw new Error('Could not find the JSZip script tag in index.html');
html = html.replace(cdn, '<script src="vendor/jszip.min.js">');
fs.writeFileSync(path.join(out, 'index.html'), html);

fs.copyFileSync(require.resolve('jszip/dist/jszip.min.js'), path.join(out, 'vendor', 'jszip.min.js'));
for (const f of ['manifest.webmanifest', 'icon-192.png', 'icon-512.png', 'apple-touch-icon.png']) {
  if (fs.existsSync(path.join(root, f))) fs.copyFileSync(path.join(root, f), path.join(out, f));
}
fs.copyFileSync(path.join(root, 'icon-512.png'), path.join(desktop, 'build', 'icon.png'));
console.log('Prepared desktop/app from index.html (' + html.length + ' bytes)');
