const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const cors = require('cors');
const fsExtra = require('fs-extra');

const app = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));
app.use('/files', express.static('uploads'));

// Create uploads folder if doesn't exist
if (!fs.existsSync('./uploads')) {
    fs.mkdirSync('./uploads');
}

// Storage configuration for multer
const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        let uploadPath = './uploads';
        
        // Create folder structure based on file type
        if (file.mimetype.startsWith('video/')) {
            uploadPath = './uploads/videos';
        } else if (file.mimetype.startsWith('image/')) {
            uploadPath = './uploads/images';
        } else {
            uploadPath = './uploads/others';
        }
        
        if (!fs.existsSync(uploadPath)) {
            fs.mkdirSync(uploadPath, { recursive: true });
        }
        
        cb(null, uploadPath);
    },
    filename: (req, file, cb) => {
        const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
        cb(null, uniqueSuffix + path.extname(file.originalname));
    }
});

// File filter - accept ALL media types
const fileFilter = (req, file, cb) => {
    const allowedTypes = [
        // Images
        'image/jpeg', 'image/png', 'image/gif', 'image/webp', 'image/bmp', 'image/svg+xml', 'image/heic',
        // Videos
        'video/mp4', 'video/mpeg', 'video/quicktime', 'video/x-msvideo', 'video/webm', 'video/ogg', 
        'video/x-matroska', 'video/mov', 'video/avi', 'video/flv', 'video/wmv', 'video/3gpp',
        // Documents
        'application/pdf', 'text/plain', 'application/msword', 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
        'application/vnd.ms-excel', 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
        // Audio
        'audio/mpeg', 'audio/wav', 'audio/ogg', 'audio/mp3'
    ];
    
    if (allowedTypes.includes(file.mimetype)) {
        cb(null, true);
    } else {
        cb(null, true); // Accept anyway - unlimited!
    }
};

const upload = multer({ 
    storage: storage,
    limits: { fileSize: Infinity }, // NO SIZE LIMIT - truly unlimited!
    fileFilter: fileFilter
});

// Store file metadata
let fileDatabase = [];
const DB_PATH = './file-database.json';

// Load existing database
if (fs.existsSync(DB_PATH)) {
    fileDatabase = JSON.parse(fs.readFileSync(DB_PATH));
}

// Save database function
function saveDatabase() {
    fs.writeFileSync(DB_PATH, JSON.stringify(fileDatabase, null, 2));
}

// Generate unique ID
function generateId() {
    return 'file_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
}

// Format file size
function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

// Get file icon based on type
function getFileIcon(mimetype, filename) {
    if (mimetype.startsWith('video/')) return '🎬';
    if (mimetype.startsWith('image/')) return '🖼️';
    if (mimetype.startsWith('audio/')) return '🎵';
    if (mimetype === 'application/pdf') return '📄';
    if (mimetype.includes('word')) return '📝';
    if (mimetype.includes('excel')) return '📊';
    if (mimetype === 'text/plain') return '📃';
    return '📁';
}

// API Routes

// Upload file (unlimited size)
app.post('/upload', upload.single('file'), (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ error: 'No file uploaded' });
        }
        
        const fileId = generateId();
        const filePath = req.file.path;
        const stats = fs.statSync(filePath);
        
        const fileMetadata = {
            id: fileId,
            name: req.file.originalname,
            filename: req.file.filename,
            path: filePath,
            size: stats.size,
            sizeFormatted: formatBytes(stats.size),
            type: req.file.mimetype,
            icon: getFileIcon(req.file.mimetype, req.file.originalname),
            uploadDate: new Date().toISOString(),
            lastModified: stats.mtime
        };
        
        fileDatabase.push(fileMetadata);
        saveDatabase();
        
        res.json({
            success: true,
            file: fileMetadata,
            message: 'File uploaded successfully!'
        });
    } catch (error) {
        console.error('Upload error:', error);
        res.status(500).json({ error: error.message });
    }
});

// Get all files
app.get('/files', (req, res) => {
    res.json(fileDatabase);
});

// Get file by ID
app.get('/file/:id', (req, res) => {
    const file = fileDatabase.find(f => f.id === req.params.id);
    if (!file) {
        return res.status(404).json({ error: 'File not found' });
    }
    res.json(file);
});

// Download file
app.get('/download/:id', (req, res) => {
    const file = fileDatabase.find(f => f.id === req.params.id);
    if (!file) {
        return res.status(404).json({ error: 'File not found' });
    }
    
    if (!fs.existsSync(file.path)) {
        return res.status(404).json({ error: 'File not found on disk' });
    }
    
    res.download(file.path, file.name);
});

// Preview file (for images/videos)
app.get('/preview/:id', (req, res) => {
    const file = fileDatabase.find(f => f.id === req.params.id);
    if (!file) {
        return res.status(404).json({ error: 'File not found' });
    }
    
    if (!fs.existsSync(file.path)) {
        return res.status(404).json({ error: 'File not found on disk' });
    }
    
    res.sendFile(path.resolve(file.path));
});

// Delete file
app.delete('/file/:id', (req, res) => {
    const fileIndex = fileDatabase.findIndex(f => f.id === req.params.id);
    if (fileIndex === -1) {
        return res.status(404).json({ error: 'File not found' });
    }
    
    const file = fileDatabase[fileIndex];
    
    // Delete from disk
    if (fs.existsSync(file.path)) {
        fs.unlinkSync(file.path);
    }
    
    // Remove from database
    fileDatabase.splice(fileIndex, 1);
    saveDatabase();
    
    res.json({ success: true, message: 'File deleted successfully' });
});

// Get storage stats
app.get('/stats', (req, res) => {
    const totalFiles = fileDatabase.length;
    const totalSize = fileDatabase.reduce((sum, file) => sum + file.size, 0);
    const totalSizeFormatted = formatBytes(totalSize);
    
    const typeStats = {
        videos: fileDatabase.filter(f => f.type.startsWith('video/')).length,
        images: fileDatabase.filter(f => f.type.startsWith('image/')).length,
        others: fileDatabase.filter(f => !f.type.startsWith('video/') && !f.type.startsWith('image/')).length
    };
    
    res.json({
        totalFiles,
        totalSize,
        totalSizeFormatted,
        typeStats,
        storagePath: path.resolve('./uploads')
    });
});

// Search files
app.get('/search', (req, res) => {
    const query = req.query.q?.toLowerCase() || '';
    const results = fileDatabase.filter(file => 
        file.name.toLowerCase().includes(query) ||
        file.type.toLowerCase().includes(query)
    );
    res.json(results);
});

// Bulk delete
app.post('/bulk-delete', (req, res) => {
    const { fileIds } = req.body;
    if (!fileIds || !fileIds.length) {
        return res.status(400).json({ error: 'No file IDs provided' });
    }
    
    let deleted = 0;
    fileIds.forEach(id => {
        const fileIndex = fileDatabase.findIndex(f => f.id === id);
        if (fileIndex !== -1) {
            const file = fileDatabase[fileIndex];
            if (fs.existsSync(file.path)) {
                fs.unlinkSync(file.path);
            }
            fileDatabase.splice(fileIndex, 1);
            deleted++;
        }
    });
    
    saveDatabase();
    res.json({ success: true, deleted, message: `Deleted ${deleted} files` });
});

// Serve index.html
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`
    ╔═══════════════════════════════════════╗
    ║     🚀 NebulaDrive is RUNNING!       ║
    ╠═══════════════════════════════════════╣
    ║  🌐 URL: http://localhost:${PORT}      ║
    ║  📁 Storage: ./uploads (UNLIMITED)    ║
    ║  💾 Database: file-database.json      ║
    ╚═══════════════════════════════════════╝
    `);
});
