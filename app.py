import os
import logging
from flask import Flask, send_from_directory, jsonify

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__, static_folder='.', static_url_path='')

# Health check endpoint (useful for monitoring)
@app.route('/api/health')
def health():
    return jsonify({"status": "ok", "message": "NebulaCloud is running"}), 200

# Serve the main index.html (your NebulaCloud app)
@app.route('/')
def serve_index():
    logger.info("Serving index.html")
    return send_from_directory('.', 'index.html')

# Serve any static files (if you add CSS/JS later)
@app.route('/<path:path>')
def serve_static(path):
    if os.path.exists(path):
        return send_from_directory('.', path)
    # If the file is not found, fallback to index.html (SPA routing)
    return send_from_directory('.', 'index.html'), 404

if __name__ == '__main__':
    # Use PORT environment variable for cloud hosting (Heroku, Render, etc.)
    port = int(os.environ.get('PORT', 5000))
    # Run with debug=False in production; set debug=True for local development
    debug = os.environ.get('FLASK_DEBUG', 'False').lower() == 'true'
    logger.info(f"Starting NebulaCloud server on port {port} (debug={debug})")
    app.run(host='0.0.0.0', port=port, debug=debug)
