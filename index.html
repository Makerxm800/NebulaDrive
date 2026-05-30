from flask import Flask, request, jsonify, send_file, send_from_directory
from flask_cors import CORS
import os
import uuid
import json
from datetime import datetime
import mimetypes

app = Flask(__name__, static_folder='.', static_url_path='')
CORS(app)

# Configuration
UPLOAD_FOLDER = 'uploads'
DB_FILE = 'files_database.json'

# Create uploads folder if not exists
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

# Load database
def load_db():
    if os.path.exists(DB_FILE):
        with open(DB_FILE, 'r') as f:
            return json.load(f)
    return {'users': {}, 'files': {}}

def save_db(data):
    with open(DB_FILE, 'w') as f:
        json.dump(data, f, indent=2)

db = load_db()

# Format file size
def format_size(bytes):
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes < 1024:
            return f"{bytes:.1f} {unit}"
        bytes /= 1024
    return f"{bytes:.1f} TB"

# Get file icon
def get_file_icon(filename):
    ext = filename.split('.')[-1].lower() if '.' in filename else ''
    icons = {
        'mp4': '🎬', 'mov': '🎬', 'avi': '🎬', 'mkv': '🎬', 'webm': '🎬',
        'jpg': '🖼️', 'jpeg': '🖼️', 'png': '🖼️', 'gif': '🖼️', 'webp': '🖼️',
        'sh': '🐧', 'bash': '🐧', 'py': '🐍', 'js': '📜', 'html': '🌐',
        'exe': '⚙️', 'msi': '⚙️', 'dmg': '⚙️',
        'pdf': '📄', 'txt': '📃', 'md': '📃', 'json': '🔧',
        'zip': '🗜️', 'rar': '🗜️', '7z': '🗜️'
    }
    return icons.get(ext, '📁')

# Routes
@app.route('/')
def serve_index():
    return send_from_directory('.', 'index.html')

@app.route('/<path:path>')
def serve_static(path):
    return send_from_directory('.', path)

@app.route('/api/signup', methods=['POST'])
def signup():
    data = request.json
    username = data.get('username')
    password = data.get('password')
    
    if username in db['users']:
        return jsonify({'error': 'Username exists'}), 400
    
    db['users'][username] = {
        'password': password,
        'created': datetime.now().isoformat(),
        'files': []
    }
    save_db(db)
    return jsonify({'success': True, 'message': 'Account created!'})

@app.route('/api/login', methods=['POST'])
def login():
    data = request.json
    username = data.get('username')
    password = data.get('password')
    
    user = db['users'].get(username)
    if user and user['password'] == password:
        return jsonify({'success': True, 'username': username})
    return jsonify({'error': 'Invalid credentials'}), 401

@app.route('/api/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': 'No file'}), 400
    
    file = request.files['file']
    username = request.form.get('username')
    
    if not username:
        return jsonify({'error': 'No user'}), 400
    
    file_id = str(uuid.uuid4())[:8]
    original_name = file.filename
    ext = original_name.split('.')[-1] if '.' in original_name else ''
    saved_name = f"{file_id}_{original_name}"
    filepath = os.path.join(UPLOAD_FOLDER, saved_name)
    
    file.save(filepath)
    size = os.path.getsize(filepath)
    
    file_info = {
        'id': file_id,
        'name': original_name,
        'size': size,
        'size_formatted': format_size(size),
        'type': mimetypes.guess_type(original_name)[0] or 'application/octet-stream',
        'icon': get_file_icon(original_name),
        'uploaded': datetime.now().isoformat(),
        'path': saved_name
    }
    
    db['files'][f"{username}_{file_id}"] = file_info
    if username in db['users']:
        db['users'][username]['files'].append(file_id)
    save_db(db)
    
    return jsonify({'success': True, 'file': file_info})

@app.route('/api/files/<username>', methods=['GET'])
def get_files(username):
    user_files = []
    for key, file_info in db['files'].items():
        if key.startswith(f"{username}_"):
            user_files.append(file_info)
    return jsonify(user_files)

@app.route('/api/download/<file_id>', methods=['GET'])
def download_file(file_id):
    for key, file_info in db['files'].items():
        if file_info['id'] == file_id:
            filepath = os.path.join(UPLOAD_FOLDER, file_info['path'])
            if os.path.exists(filepath):
                return send_file(filepath, as_attachment=True, download_name=file_info['name'])
    return jsonify({'error': 'File not found'}), 404

@app.route('/api/preview/<file_id>', methods=['GET'])
def preview_file(file_id):
    for key, file_info in db['files'].items():
        if file_info['id'] == file_id:
            filepath = os.path.join(UPLOAD_FOLDER, file_info['path'])
            if os.path.exists(filepath):
                return send_file(filepath, mimetype=file_info['type'])
    return jsonify({'error': 'File not found'}), 404

@app.route('/api/delete/<username>/<file_id>', methods=['DELETE'])
def delete_file(username, file_id):
    key = f"{username}_{file_id}"
    if key in db['files']:
        filepath = os.path.join(UPLOAD_FOLDER, db['files'][key]['path'])
        if os.path.exists(filepath):
            os.remove(filepath)
        del db['files'][key]
        if username in db['users']:
            db['users'][username]['files'] = [f for f in db['users'][username]['files'] if f != file_id]
        save_db(db)
        return jsonify({'success': True})
    return jsonify({'error': 'File not found'}), 404

@app.route('/api/clear/<username>', methods=['DELETE'])
def clear_all_files(username):
    to_delete = []
    for key, file_info in db['files'].items():
        if key.startswith(f"{username}_"):
            filepath = os.path.join(UPLOAD_FOLDER, file_info['path'])
            if os.path.exists(filepath):
                os.remove(filepath)
            to_delete.append(key)
    
    for key in to_delete:
        del db['files'][key]
    
    if username in db['users']:
        db['users'][username]['files'] = []
    save_db(db)
    
    return jsonify({'success': True})

if __name__ == '__main__':
    print("\n" + "="*50)
    print("☁️  NEBULA DRIVE CLOUD SERVICE")
    print("="*50)
    print(f"📍 Server: http://localhost:5000")
    print(f"📁 Uploads: ./{UPLOAD_FOLDER}")
    print("="*50 + "\n")
    app.run(debug=True, host='0.0.0.0', port=5000)
