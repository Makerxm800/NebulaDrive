const API_URL = window.location.origin;

let currentFiles = [];
let currentFilter = 'all';
let currentView = 'grid';

const uploadArea = document.getElementById('uploadArea');
const fileInput = document.getElementById('fileInput');
const filesContainer = document.getElementById('filesContainer');
const searchInput = document.getElementById('searchInput');
const totalFilesEl = document.getElementById('totalFiles');
const totalSizeEl = document.getElementById('totalSize');
const uploadProgress = document.getElementById('uploadProgress');
const progressFill = document.querySelector('.progress-fill');
const progressText = document.querySelector('.progress-text');
const clearAllBtn = document.getElementById('clearAllBtn');
const modal = document.getElementById('previewModal');
const modalBody = document.getElementById('modalBody');
const closeModal = document.querySelector('.close');

loadFiles();
loadStats();

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
        for (const file of currentFiles) {
            await fetch(`${API_URL}/file/${file.id}`, { method: 'DELETE' });
        }
        loadFiles();
        loadStats();
    }
});

closeModal.onclick = () => modal.style.display = 'none';
window.onclick = (e) => {
    if (e.target === modal) modal.style.display = 'none';
};

async function uploadMultipleFiles(files) {
    for (const file of files) {
        await uploadFile(file);
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
        
        xhr.send(formData);
        await new Promise((resolve) => { xhr.onloadend = resolve; });
        
        showNotification(`✅ ${file.name} uploaded!`, 'success');
    } catch (error) {
        showNotification(`❌ Failed to upload ${file.name}`, 'error');
    } finally {
        uploadProgress.style.display = 'none';
        progressFill.style.width = '0%';
    }
}

async function loadFiles() {
    try {
        const response = await fetch(`${API_URL}/files`);
        currentFiles = await response.json();
        filterFiles();
    } catch (error) {
        filesContainer.innerHTML = '<div class="loading">❌ Failed to load files. Make sure server is running!</div>';
    }
}

async function loadStats() {
    try {
        const response = await fetch(`${API_URL}/stats`);
        const stats = await response.json();
        totalFilesEl.textContent = stats.totalFiles;
        totalSizeEl.textContent = stats.totalSizeFormatted;
    } catch (error) {
        console.error('Load stats error:', error);
    }
}

function filterFiles() {
    let filtered = [...currentFiles];
    const searchTerm = searchInput.value.toLowerCase();
    if (searchTerm) {
        filtered = filtered.filter(f => f.name.toLowerCase().includes(searchTerm));
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
        filesContainer.innerHTML = files.map(file => `
            <div class="file-card">
                <div class="file-icon">${file.icon}</div>
                <div class="file-name">${file.name.substring(0, 30)}${file.name.length > 30 ? '...' : ''}</div>
                <div class="file-size">${file.sizeFormatted}</div>
                <div class="file-actions">
                    <button class="download-btn" data-id="${file.id}">⬇️ Download</button>
                    <button class="preview-btn" data-id="${file.id}">👁️ Preview</button>
                    <button class="delete-btn" data-id="${file.id}">🗑️</button>
                </div>
            </div>
        `).join('');
    } else {
        filesContainer.innerHTML = files.map(file => `
            <div class="file-card">
                <div class="file-icon">${file.icon}</div>
                <div class="file-name">${file.name}</div>
                <div class="file-size">${file.sizeFormatted}</div>
                <div class="file-actions">
                    <button class="download-btn" data-id="${file.id}">⬇️ Download</button>
                    <button class="preview-btn" data-id="${file.id}">👁️ Preview</button>
                    <button class="delete-btn" data-id="${file.id}">🗑️</button>
                </div>
            </div>
        `).join('');
    }
    
    document.querySelectorAll('.download-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            e.stopPropagation();
            window.location.href = `${API_URL}/download/${btn.dataset.id}`;
        });
    });
    
    document.querySelectorAll('.preview-btn').forEach(btn => {
        btn.addEventListener('click', async (e) => {
            e.stopPropagation();
            const file = currentFiles.find(f => f.id === btn.dataset.id);
            if (file.type.startsWith('image/')) {
                modalBody.innerHTML = `<img src="${API_URL}/preview/${file.id}" alt="${file.name}">`;
            } else if (file.type.startsWith('video/')) {
                modalBody.innerHTML = `<video controls autoplay style="width:100%"><source src="${API_URL}/preview/${file.id}" type="${file.type}"></video>`;
            } else {
                modalBody.innerHTML = `<p>Preview not available.</p>`;
            }
            modal.style.display = 'block';
        });
    });
    
    document.querySelectorAll('.delete-btn').forEach(btn => {
        btn.addEventListener('click', async (e) => {
            e.stopPropagation();
            if (confirm('Delete this file?')) {
                await fetch(`${API_URL}/file/${btn.dataset.id}`, { method: 'DELETE' });
                loadFiles();
                loadStats();
                showNotification('✅ File deleted!', 'success');
            }
        });
    });
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
    setTimeout(() => notification.remove(), 3000);
}
