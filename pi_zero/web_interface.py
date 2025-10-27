#!/usr/bin/env python3
"""
Pi Zero Web Interface for Winding Controller
Provides web-based control interface
"""

from flask import Flask, render_template, jsonify, request
import json
import threading
import time
from winding_controller import WindingController, WindingParams, WindingState

app = Flask(__name__)
controller = WindingController()

# Global status for web interface
current_status = {
    "connected": False,
    "state": "idle",
    "turns": 0,
    "target_turns": 1000,
    "rpm": 0.0,
    "layer": 0,
    "position": 0.0,
    "progress": 0.0
}

def update_status(ctrl):
    """Update global status from controller"""
    global current_status
    progress = ctrl.get_progress()
    current_status.update({
        "connected": ctrl.serial_conn is not None and ctrl.serial_conn.is_open,
        "state": progress["state"],
        "turns": progress["current_turns"],
        "target_turns": progress["target_turns"],
        "rpm": progress["current_rpm"],
        "layer": progress["current_layer"],
        "position": progress["traverse_position"],
        "progress": progress["progress_percent"]
    })

@app.route('/')
def index():
    """Main control page"""
    return render_template('index.html')

@app.route('/api/status')
def api_status():
    """Get current status"""
    return jsonify(current_status)

@app.route('/api/connect', methods=['POST'])
def api_connect():
    """Connect to Pico"""
    try:
        if controller.connect():
            controller.add_status_callback(update_status)
            return jsonify({"success": True, "message": "Connected to Pico successfully"})
        else:
            return jsonify({"success": False, "message": "Failed to connect to Pico. Check UART connection."})
    except Exception as e:
        return jsonify({"success": False, "message": f"Connection error: {str(e)}"})

@app.route('/api/disconnect', methods=['POST'])
def api_disconnect():
    """Disconnect from Pico"""
    controller.disconnect()
    return jsonify({"success": True, "message": "Disconnected from Pico"})

@app.route('/api/home', methods=['POST'])
def api_home():
    """Home all axes"""
    if controller.home_all_axes():
        return jsonify({"success": True, "message": "Homing completed"})
    else:
        return jsonify({"success": False, "message": "Homing failed"})

@app.route('/api/start', methods=['POST'])
def api_start():
    """Start winding"""
    data = request.get_json() or {}
    
    # Parse parameters
    params = WindingParams(
        target_turns=int(data.get('turns', 1000)),
        spindle_rpm=float(data.get('rpm', 300)),
        wire_diameter_mm=float(data.get('wire_diameter', 0.064)),
        bobbin_width_mm=float(data.get('bobbin_width', 12.0)),
        offset_mm=float(data.get('offset', 22.0))
    )
    
    if controller.start_winding(params):
        return jsonify({"success": True, "message": "Winding started"})
    else:
        return jsonify({"success": False, "message": "Failed to start winding"})

@app.route('/api/pause', methods=['POST'])
def api_pause():
    """Pause winding"""
    if controller.pause_winding():
        return jsonify({"success": True, "message": "Winding paused"})
    else:
        return jsonify({"success": False, "message": "Failed to pause winding"})

@app.route('/api/resume', methods=['POST'])
def api_resume():
    """Resume winding"""
    if controller.resume_winding():
        return jsonify({"success": True, "message": "Winding resumed"})
    else:
        return jsonify({"success": False, "message": "Failed to resume winding"})

@app.route('/api/stop', methods=['POST'])
def api_stop():
    """Stop winding"""
    if controller.stop_winding():
        return jsonify({"success": True, "message": "Winding stopped"})
    else:
        return jsonify({"success": False, "message": "Failed to stop winding"})

@app.route('/api/emergency', methods=['POST'])
def api_emergency():
    """Emergency stop"""
    if controller.emergency_stop():
        return jsonify({"success": True, "message": "EMERGENCY STOP ACTIVATED"})
    else:
        return jsonify({"success": False, "message": "Emergency stop failed"})

@app.route('/api/reset', methods=['POST'])
def api_reset():
    """Reset emergency stop"""
    if controller.reset_emergency_stop():
        return jsonify({"success": True, "message": "Emergency stop reset"})
    else:
        return jsonify({"success": False, "message": "Failed to reset emergency stop"})

if __name__ == '__main__':
    print("🌐 Starting Pi Zero Winding Controller Web Interface")
    print("📱 Open your browser to: http://pi-zero-ip:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)
