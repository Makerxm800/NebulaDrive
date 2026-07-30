import os
from flask import Flask, send_file

app = Flask(__name__)

@app.route('/')
def serve_index():
    # Try to serve NebulaCloud.html first, fallback to index.html
    if os.path.exists('NebulaCloud.html'):
        return send_file('NebulaCloud.html')
    elif os.path.exists('index.html'):
        return send_file('index.html')
    else:
        return "No HTML file found. Please upload index.html or NebulaCloud.html", 404

@app.route('/<path:path>')
def serve_static(path):
    # Serve any other static file (favicon.ico, etc.)
    if os.path.exists(path):
        return send_file(path)
    return "File not found", 404

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    app.run(host='0.0.0.0', port=port, debug=True)
