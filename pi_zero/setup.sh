#!/bin/bash
# Pi Zero Winding Controller Setup Script

echo "🔄 Pi Zero Winding Controller Setup"
echo "=================================="

# Update system
echo "📦 Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install Python dependencies
echo "🐍 Installing Python dependencies..."
sudo apt install -y python3-pip python3-venv

# Create virtual environment
echo "🔧 Creating virtual environment..."
python3 -m venv venv
source venv/bin/activate

# Install Python packages
echo "📚 Installing Python packages..."
pip install -r requirements.txt

# Enable UART
echo "🔌 Enabling UART..."
sudo raspi-config nonint do_serial 0

# Add user to dialout group for UART access
echo "👤 Adding user to dialout group..."
sudo usermod -a -G dialout $USER

# Create systemd service for auto-start
echo "🚀 Creating systemd service..."
sudo tee /etc/systemd/system/winding-controller.service > /dev/null <<EOF
[Unit]
Description=Pi Zero Winding Controller
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/Pi-Zero-SKR-Pico-PUWinder/pi_zero
Environment=PATH=/home/pi/Pi-Zero-SKR-Pico-PUWinder/pi_zero/venv/bin
ExecStart=/home/pi/Pi-Zero-SKR-Pico-PUWinder/pi_zero/venv/bin/python web_interface.py
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable winding-controller.service

# Set permissions
echo "🔐 Setting permissions..."
chmod +x *.py
chmod +x setup.sh

echo "✅ Setup complete!"
echo ""
echo "🎮 Usage:"
echo "  Web Interface: python web_interface.py"
echo "  Simple Control: python simple_control.py"
echo "  Auto-start: sudo systemctl start winding-controller"
echo ""
echo "📱 Web interface will be available at: http://pi-zero-ip:5000"
echo "🔌 Make sure Pico is connected via UART (GPIO 14,15)"
echo ""
echo "🔄 Reboot recommended to enable UART changes"
