#!/usr/bin/env python3
"""
Simple web test to check if Flask is working
"""

from flask import Flask, jsonify

app = Flask(__name__)

@app.route('/')
def hello():
    return jsonify({"message": "Hello! Web interface is working!", "status": "ok"})

@app.route('/test')
def test():
    return jsonify({"test": "success", "port": 5000})

if __name__ == '__main__':
    print("🧪 Starting simple web test")
    print("📱 Test URLs:")
    print("   http://localhost:8080")
    print("   http://127.0.0.1:8080")
    print("   http://192.168.45.156:8080")
    print("   Press CTRL+C to quit")
    
    app.run(host='0.0.0.0', port=8080, debug=False)
