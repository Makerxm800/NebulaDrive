// API Configuration
const API_URL = 'http://localhost:5000/api';
let currentUser = null;
let currentFiles = [];
let currentView = 'grid';

// DOM Elements
const authScreen = document.getElementById('authScreen');
const mainApp = document.getElementById('mainApp');
const filesGrid = document.getElementById('filesGrid');
const searchInput = document.getElementById('searchInput');

// Auth Functions
async function login() {
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;
    
    if (!username || !password) {
        showToast('Please enter username and password', 'error');
        return;
    }
    
    try {
        const res = await fetch(`${API_URL}/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
        
        const data = await res.json();
        if (data.success) {
            currentUser = username;
            localStorage.setItem('nebula_user', username);
            showMainApp();
            loadFiles();
            showToast(`Welcome back, ${username}!`);
        } else {
            showToast(data.error || 'Login failed', 'error');
        }
    } catch (err) {
        showToast('Server error. Make sure app.py is running!', 'error');
    }
}

async function signup() {
    const username = document.getElementById('signupUsername').value;
    const password = document.getElementById('signupPassword').value;
    
    if (!username || !password) {
        showToast('Please enter username and password', 'error');
        return;
    }
    
    try {
        const res = await fetch(`${API_URL}/signup`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ username, password })
        });
        
        const data = await res.json();
        if (data.success) {
            showToast('Account created! Please login.');
            switchTab('login');
            document.getElementById('loginUsername').value = username;
        } else {
            showToast(data.error || 'Signup failed', 'error');
        }
    } catch (err) {
        showToast('Server error', 'error');
    }
}

function logout() {
    currentUser = null;
    localStorage.removeItem('nebula_user');
    authScreen.style.display = 'flex';
    mainApp.style.display = 'none';
    currentFiles = [];
    showToast('Logged out');
}

function showMainApp() {
    authScreen.style.display = 'none';
    mainApp.style.display = 'block';
    document.getElementById('usernameDisplay').textContent = currentUser;
}

// File Functions
async function uploadFile(file) {
    const formData = new FormData();
    formData.append('file', file);
    formData.append('username', currentUser);
    
    const progressContainer = document.getElementById('progressContainer');
    const progressFill = document.getElementById('progressFill');
    const progressText = document.getElementById('progressText');
    
    progressContainer.style.display = 'block';
    
    try {
        const xhr = new XMLHttpRequest();
        xhr.open('POST', `${API_URL}/upload`);
        
        xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
                const percent = Math.round((e.loaded / e.total) * 100);
                progressFill.style.width = `${percent}%`;
                progressText.textContent = `${percent}%`;
            }
        };
        
        xhr.onload = () => {
            const data = JSON.parse(xhr.response);
            if (data.success) {
                currentFiles.push(data.file);
                updateDisplay();
                updateStats();
                showToast(`✓ ${file.name} uploaded`);
            } else {
                showToast(`✗ ${file.name} failed`, 'error');
            }
            setTimeout(() => {
                progressContainer.style.display = 'none';
                progressFill.style.width = '0%';
            }, 500);
        };
        
        xhr.send(formData);
    } catch (err) {
        showToast(`✗ ${file.name} failed`, 'error');
        progressContainer.style.display = 'none';
    }
}

async function loadFiles() {
    try {
        const res = await fetch(`${API_URL}/files/${currentUser}`);
        currentFiles = await res.json();
        updateDisplay();
        updateStats();
    } catch (err) {
        console.error('Load files error:', err);
    }
}

async function downloadFile(fileId, fileName) {
    window.open(`${API_URL}/download/${fileId}`, '_blank');
    showToast(`↓ Downloading ${fileName}`);
}

async function previewFile(fileId, fileType) {
    if (fileType.startsWith('image/') || fileType.startsWith('video/')) {
        window.open(`${API_URL}/preview/${fileId}`, '_blank');
    } else {
        showToast('Preview not available for this file type', 'error');
    }
}

async function deleteFile(fileId, fileName) {
    if (confirm(`Delete "${fileName}"?`)) {
        try {
            await fetch(`${API_URL}/delete/${currentUser}/${fileId}`, { method: 'DELETE' });
            currentFiles = currentFiles.filter(f => f.id !== fileId);
            updateDisplay();
            updateStats();
            showToast(`✓ ${fileName} deleted`);
        } catch (err) {
            showToast('Delete failed', 'error');
        }
    }
}

async function clearAllFiles() {
    if (confirm('⚠️ Delete ALL your files? This cannot be undone!')) {
        try {
            await fetch(`${API_URL}/clear/${currentUser}`, { method: 'DELETE' });
            currentFiles = [];
            updateDisplay();
            updateStats();
            showToast('✓ All files cleared');
        } catch (err) {
            showToast('Clear failed', 'error');
        }
    }
}

// UI Functions
function updateDisplay() {
    const searchTerm = searchInput.value.toLowerCase();
    let filtered = currentFiles;
    if (searchTerm) {
        filtered = currentFiles.filter(f => f.name.toLowerCase().includes(searchTerm));
    }
    
    filesGrid.className = `files-grid ${currentView === 'list' ? 'list-view' : ''}`;
    
    if (filtered.length === 0) {
        filesGrid.innerHTML = `
            <div class="empty-state">
                <div class="empty-emoji">☁️</div>
                <h3>${searchTerm ? 'No files found' : 'Your cloud is empty'}</h3>
                <p>${searchTerm ? 'Try another search' : 'Drag & drop files to get started'}</p>
            </div>
        `;
        return;
    }
    
    if (currentView === 'grid') {
        filesGrid.innerHTML = filtered.map(file => `
            <div class="file-card">
                <div class="file-emoji">${file.icon}</div>
                <div class="file-name">${file.name.length > 35 ? file.name.slice(0,35)+'...' : file.name}</div>
                <div class="file-size">${file.size_formatted}</div>
                <div class="file-buttons">
                    <button class="file-btn preview" onclick='previewFile("${file.id}", "${file.type}")'>👁️</button>
                    <button class="file-btn download" onclick='downloadFile("${file.id}", "${file.name}")'>⬇️</button>
                    <button class="file-btn delete" onclick='deleteFile("${file.id}", "${file.name}")'>🗑️</button>
                </div>
            </div>
        `).join('');
    } else {
        filesGrid.innerHTML = filtered.map(file => `
            <div class="file-card">
                <div class="file-emoji">${file.icon}</div>
                <div class="file-info">
                    <div class="file-name">${file.name}</div>
                    <div class="file-size">${file.size_formatted}</div>
                </div>
                <div class="file-buttons">
                    <button class="file-btn preview" onclick='previewFile("${file.id}", "${file.type}")'>👁️</button>
                    <button class="file-btn download" onclick='downloadFile("${file.id}", "${file.name}")'>⬇️</button>
                    <button class="file-btn delete" onclick='deleteFile("${file.id}", "${file.name}")'>🗑️</button>
                </div>
            </div>
        `).join('');
    }
}

function updateStats() {
    const total = currentFiles.length;
    const totalSize = currentFiles.reduce((sum, f) => sum + f.size, 0);
    const videos = currentFiles.filter(f => f.type.startsWith('video/')).length;
    const images = currentFiles.filter(f => f.type.startsWith('image/')).length;
    
    document.getElementById('totalFiles').textContent = total;
    document.getElementById('totalSize').textContent = formatBytes(totalSize);
    document.getElementById('videoCount').textContent = videos;
    document.getElementById('imageCount').textContent = images;
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
}

function showToast(message, type = 'success') {
    const toast = document.createElement('div');
    toast.className = 'toast';
    toast.textContent = message;
    document.body.appendChild(toast);
    setTimeout(() => toast.remove(), 2500);
}

function switchTab(tab) {
    document.querySelectorAll('.auth-tab').forEach(t => t.classList.remove('active'));
    document.querySelector(`.auth-tab[data-tab="${tab}"]`).classList.add('active');
    
    document.getElementById('loginForm').style.display = tab === 'login' ? 'block' : 'none';
    document.getElementById('signupForm').style.display = tab === 'signup' ? 'block' : 'none';
}

// Event Listeners
document.getElementById('loginBtn').onclick = login;
document.getElementById('signupBtn').onclick = signup;
document.getElementById('logoutBtn').onclick = logout;
document.getElementById('clearBtn').onclick = clearAllFiles;

document.querySelectorAll('.auth-tab').forEach(btn => {
    btn.onclick = () => switchTab(btn.dataset.tab);
});

searchInput.oninput = updateDisplay;

document.querySelectorAll('.view-btn').forEach(btn => {
    btn.onclick = () => {
        document.querySelectorAll('.view-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        currentView = btn.dataset.view;
        updateDisplay();
    };
});

// Upload handling
const uploadArea = document.getElementById('uploadArea');
const fileInput = document.getElementById('fileInput');

uploadArea.onclick = () => fileInput.click();
uploadArea.ondragover = (e) => {
    e.preventDefault();
    uploadArea.style.borderColor = '#667eea';
    uploadArea.style.background = 'rgba(102,126,234,0.05)';
};
uploadArea.ondragleave = () => {
    uploadArea.style.borderColor = 'rgba(255,255,255,0.15)';
    uploadArea.style.background = 'rgba(255,255,255,0.03)';
};
uploadArea.ondrop = async (e) => {
    e.preventDefault();
    uploadArea.style.borderColor = 'rgba(255,255,255,0.15)';
    uploadArea.style.background = 'rgba(255,255,255,0.03)';
    const files = Array.from(e.dataTransfer.files);
    for (let file of files) await uploadFile(file);
};
fileInput.onchange = async (e) => {
    for (let file of Array.from(e.target.files)) await uploadFile(file);
    fileInput.value = '';
};

// Check saved user
const savedUser = localStorage.getItem('nebula_user');
if (savedUser) {
    currentUser = savedUser;
    showMainApp();
    loadFiles();
}

// Make functions global for onclick
window.previewFile = previewFile;
window.downloadFile = downloadFile;
window.deleteFile = deleteFile;
