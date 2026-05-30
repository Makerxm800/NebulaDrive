const express = require('express');
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const cors = require('cors');

const app = express();
const PORT = 3000;

app.use(cors());
app.use(express.json());
app.use(express.static('public'));

if (!fs.existsSync('./uploads')) {
    fs.mkdirSync('./uploads');
}

const storage = multer.diskStorage({
    destination: (req, file, cb) => {
        let uploadPath = './uploads';
        if (file.mimetype.startsWith('video/')) {
            uploadPath = './uploads/videos';
        } else if (file.mimetype.startsWith('image/')) {
            uploadPath = './uploads/images';
        } else {
            uploadPath = './uploads/others';
        }
        if (!fs.existsSync(uploadPath)) fs.mkdirSync(uploadPath, { recursive: true });
        cb(null, uploadPath);
    },
    filename: (req, file, cb) => {
        const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
        cb(null, uniqueSuffix + path.extname(file.originalname));
    }
});

const upload = multer({ storage: storage, limits: { fileSize: Infinity } });

let fileDatabase = [];
const DB_PATH = './file-database.json';

if (fs.existsSync(DB_PATH)) {
    fileDatabase = JSON.parse(fs.readFileSync(DB_PATH));
}

function saveDatabase() {
    fs.writeFileSync(DB_PATH, JSON.stringify(fileDatabase, null, 2));
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function getFileIcon(mimetype) {
    if (mimetype.startsWith('video/')) return '🎬';
    if (mimetype.startsWith('image/')) return '🖼️';
    if (mimetype.startsWith('audio/')) return '🎵';
    return '📁';
}

app.post('/upload', upload.single('file'), (req, res) => {
    try {
        if (!req.file) return res.status(400).json({ error: 'No file uploaded' });
        
        const fileId = 'file_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
        const stats = fs.statSync(req.file.path);
        
        const fileMetadata = {
            id: fileId,
            name: req.file.originalname,
            filename: req.file.filename,
            path: req.file.path,
            size: stats.size,
            sizeFormatted: formatBytes(stats.size),
            type: req.file.mimetype,
            icon: getFileIcon(req.file.mimetype),
            uploadDate: new Date().toISOString()
        };
        
        fileDatabase.push(fileMetadata);
        saveDatabase();
        
        res.json({ success: true, file: fileMetadata });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/files', (req, res) => {
    res.json(fileDatabase);
});

app.get('/download/:id', (req, res) => {
    const file = fileDatabase.find(f => f.id === req.params.id);
    if (!file) return res.status(404).json({ error: 'File not found' });
    res.download(file.path, file.name);
});

app.get('/preview/:id', (req, res) => {
    const file = fileDatabase.find(f => f.id === req.params.id);
    if (!file) return res.status(404).json({ error: 'File not found' });
    res.sendFile(path.resolve(file.path));
});

app.delete('/file/:id', (req, res) => {
    const fileIndex = fileDatabase.findIndex(f => f.id === req.params.id);
    if (fileIndex === -1) return res.status(404).json({ error: 'File not found' });
    
    const file = fileDatabase[fileIndex];
    if (fs.existsSync(file.path)) fs.unlinkSync(file.path);
    fileDatabase.splice(fileIndex, 1);
    saveDatabase();
    
    res.json({ success: true });
});

app.get('/stats', (req, res) => {
    const totalFiles = fileDatabase.length;
    const totalSize = fileDatabase.reduce((sum, file) => sum + file.size, 0);
    res.json({
        totalFiles,
        totalSizeFormatted: formatBytes(totalSize),
        typeStats: {
            videos: fileDatabase.filter(f => f.type.startsWith('video/')).length,
            images: fileDatabase.filter(f => f.type.startsWith('image/')).length
        }
    });
});

app.listen(PORT, () => {
    console.log(`\n✅ NebulaDrive is running!`);
    console.log(`🌐 Open: http://localhost:${PORT}`);
    console.log(`📁 Storage: ./uploads (UNLIMITED)\n`);
});
