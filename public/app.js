const API_URL = window.location.origin;

let currentFiles = [];
let selectedFiles = new Set();
let currentFilter = 'all';
let currentView = 'grid';

// DOM Elements
const uploadArea = document.getElementById('uploadArea');
const fileInput = document.getElementById('fileInput');
const filesContainer = document.getElementById('filesContainer');
const searchInput = document.getElementById('searchInput');
const totalFilesEl = document.getElementById('totalFiles');
const totalSizeEl = document.getElementById('totalSize');
const videoCountEl = document.getElementById('videoCount');
const imageCountEl = document.getElementById('imageCount');
const uploadProgress = document.getElementById('uploadProgress');
const progressFill = document.querySelector('.progress-fill');
const progressText = document.querySelector('.progress-text');
const clearAllBtn = document.getElementById('clearAllBtn');
const modal = document.getElementById('previewModal');
const modalBody = document.getElementById('modalBody');
const closeModal = document.querySelector('.close');

// Initialize
loadFiles();
loadStats();

// Event Listeners
uploadArea.addEventListener('click', () => fileInput.click());
uploadArea.addEventListener('dragover', (e) => {
    e.preventDefault();
    uploadArea.style.background = '#f8f9ff';
});
uploadArea.addEventListener('dragleave', () => {
    uploadArea.style.background = 'white';
});
uploadArea.addEventListener('drop', async (e) => {
    e.preventDefault();
    uploadArea.style.background = 'white';
    const files = Array.from(e.dataTransfer.files);
    await uploadMultipleFiles(files);
});

fileInput.addEventListener('change', async (e) => {
    const files = Array.from(e.target.files);
    await uploadMultipleFiles(files);
    fileInput.value = '';
});

searchInput.addEventListener('input', filterFiles);

document.querySelectorAll('.filter-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        currentFilter = btn.dataset.filter;
        filterFiles();
    });
});

document.querySelectorAll('.view-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.view-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        currentView = btn.dataset.view;
        renderFiles();
    });
});

clearAllBtn.addEventListener('click', async () => {
    if (confirm('⚠️ WARNING: This will delete ALL your files. Are you sure?')) {
        const fileIds = currentFiles.map(f => f.id);
        await bulkDeleteFiles(fileIds);
        loadFiles();
        loadStats();
    }
});

closeModal.onclick = () => modal.style.display = 'none';
window.onclick = (e) => {
    if (e.target === modal) modal.style.display = 'none';
};

// File Upload Functions
async function uploadMultipleFiles(files) {
    for (let i = 0; i < files.length; i++) {
        await uploadFile(files[i]);
    }
    loadFiles();
    loadStats();
}

async function uploadFile(file) {
    const formData = new FormData();
    formData.append('file', file);
    
    uploadProgress.style.display = 'block';
    
    try {
        const xhr = new XMLHttpRequest();
        xhr.open('POST', `${API_URL}/upload`);
        
        xhr.upload.onprogress = (e) => {
            if (e.lengthComputable) {
                const percent = (e.loaded / e.total) * 100;
                progressFill.style.width = `${percent}%`;
                progressText.textContent = `Uploading ${file.name}: ${Math.round(percent)}%`;
            }
        };
        
        xhr.onload = () => {
            if (xhr.status === 200) {
                showNotification(`✅ ${file.name} uploaded!`, 'success');
            } else {
                showNotification(`❌ Failed to upload ${file.name}`, 'error');
            }
        };
        
        xhr.send(formData);
        
        await new Promise((resolve) => {
            xhr.onloadend = resolve;
        });
    } catch (error) {
        console.error('Upload error:', error);
        showNotification(`❌ Error uploading ${file.name}`, 'error');
    } finally {
        uploadProgress.style.display = 'none';
        progressFill.style.width = '0%';
    }
}

// Load Files
async function loadFiles() {
    try {
        const response = await fetch(`${API_URL}/files`);
        currentFiles = await response.json();
        filterFiles();
    } catch (error) {
        console.error('Load files error:', error);
        filesContainer.innerHTML = '<div class="loading">❌ Failed to load files. Make sure server is running!</div>';
    }
}

async function loadStats() {
    try {
        const response = await fetch(`${API_URL}/stats`);
        const stats = await response.json();
        totalFilesEl.textContent = stats.totalFiles;
        totalSizeEl.textContent = stats.totalSizeFormatted;
        videoCountEl.textContent = stats.typeStats.videos;
        imageCountEl.textContent = stats.typeStats.images;
    } catch (error) {
        console.error('Load stats error:', error);
    }
}

function filterFiles() {
    let filtered = [...currentFiles];
    
    // Apply search filter
    const searchTerm = searchInput.value.toLowerCase();
    if (searchTerm) {
        filtered = filtered.filter(f => f.name.toLowerCase().includes(searchTerm));
    }
    
    // Apply type filter
    if (currentFilter === 'video') {
        filtered = filtered.filter(f => f.type.startsWith('video/'));
    } else if (currentFilter === 'image') {
        filtered = filtered.filter(f => f.type.startsWith('image/'));
    }
    
    renderFiles(filtered);
}

function renderFiles(files = currentFiles) {
    if (!files.length) {
        filesContainer.innerHTML = '<div class="loading">✨ No files yet. Upload something!</div>';
        filesContainer.className = `files-container ${currentView}-view`;
        return;
    }
    
    filesContainer.className = `files-container ${currentView}-view`;
    
    if (currentView === 'grid') {
        filesContainer.innerHTML = files.map(file => createGridCard(file)).join('');
    } else {
        filesContainer.innerHTML = files.map(file => createListCard(file)).join('');
    }
    
    // Attach event listeners
    document.querySelectorAll('.download-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            downloadFile(btn.dataset.id);
        });
    });
    
    document.querySelectorAll('.preview-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            previewFile(btn.dataset.id);
        });
    });
    
    document.querySelectorAll('.delete-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            deleteFile(btn.dataset.id);
        });
    });
    
    document.querySelectorAll('.file-checkbox').forEach(cb => {
        cb.addEventListener('change', (e) => {
            e.stopPropagation();
            const fileId = cb.dataset.id;
            if (cb.checked) {
                selectedFiles.add(fileId);
            } else {
                selectedFiles.delete(fileId);
            }
        });
    });
}

function createGridCard(file) {
    return `
        <div class="file-card">
            <input type="checkbox" class="file-checkbox" data-id="${file.id}">
            <div class="file-icon">${file.icon}</div>
            <div class="file-name" title="${file.name}">${file.name.substring(0, 30)}${file.name.length > 30 ? '...' : ''}</div>
            <div class="file-size">${file.sizeFormatted}</div>
            <div class="file-actions">
                <button class="download-btn" data-id="${file.id}">⬇️ Download</button>
                <button class="preview-btn" data-id="${file.id}">👁️ Preview</button>
                <button class="delete-btn" data-id="${file.id}">🗑️</button>
            </div>
        </div>
    `;
}

function createListCard(file) {
    return `
        <div class="file-card">
            <input type="checkbox" class="file-checkbox" data-id="${file.id}">
            <div class="file-icon">${file.icon}</div>
            <div class="file-name" title="${file.name}">${file.name}</div>
            <div class="file-size">${file.sizeFormatted}</div>
            <div class="file-actions">
                <button class="download-btn" data-id="${file.id}">⬇️ Download</button>
                <button class="preview-btn" data-id="${file.id}">👁️ Preview</button>
                <button class="delete-btn" data-id="${file.id}">🗑️</button>
            </div>
        </div>
    `;
}

async function downloadFile(fileId) {
    try {
        const file = currentFiles.find(f => f.id === fileId);
        window.location.href = `${API_URL}/download/${fileId}`;
        showNotification(`⬇️ Downloading ${file.name}...`, 'success');
    } catch (error) {
        showNotification('❌ Download failed', 'error');
    }
}

async function previewFile(fileId) {
    const file = currentFiles.find(f => f.id === fileId);
    if (!file) return;
    
    const previewUrl = `${API_URL}/preview/${fileId}`;
    
    if (file.type.startsWith('image/')) {
        modalBody.innerHTML = `<img src="${previewUrl}" alt="${file.name}">`;
    } else if (file.type.startsWith('video/')) {
        modalBody.innerHTML = `<video controls autoplay style="width:100%"><source src="${previewUrl}" type="${file.type}"></video>`;
    } else {
        modalBody.innerHTML = `<p>Preview not available for this file type.</p><a href="${API_URL}/download/${fileId}" class="download-btn">Download instead</a>`;
    }
    
    modal.style.display = 'block';
}

async function deleteFile(fileId) {
    if (!confirm('Are you sure you want to delete this file?')) return;
    
    try {
        const response = await fetch(`${API_URL}/file/${fileId}`, {
            method: 'DELETE'
        });
        
        if (response.ok) {
            showNotification('✅ File deleted!', 'success');
            loadFiles();
            loadStats();
        }
    } catch (error) {
        showNotification('❌ Delete failed', 'error');
    }
}

async function bulkDeleteFiles(fileIds) {
    try {
        const response = await fetch(`${API_URL}/bulk-delete`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ fileIds })
        });
        
        if (response.ok) {
            showNotification(`✅ Deleted ${fileIds.length} files!`, 'success');
            selectedFiles.clear();
        }
    } catch (error) {
        showNotification('❌ Bulk delete failed', 'error');
    }
}

function showNotification(message, type) {
    const notification = document.createElement('div');
    notification.textContent = message;
    notification.style.cssText = `
        position: fixed;
        bottom: 20px;
        right: 20px;
        padding: 15px 20px;
        background: ${type === 'success' ? '#4caf50' : '#f44336'};
        color: white;
        border-radius: 8px;
        z-index: 10000;
        animation: slideIn 0.3s ease;
    `;
    
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 3000);
}

// Auto-refresh every 30 seconds
setInterval(() => {
    if (document.visibilityState === 'visible') {
        loadFiles();
        loadStats();
    }
}, 30000);
