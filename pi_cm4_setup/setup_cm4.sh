#!/bin/bash
# Pi CM4 Touchscreen Setup Script

echo "🔄 Pi CM4 Touchscreen Setup"
echo "============================"

# Update system
echo "📦 Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install required packages
echo "🐍 Installing Python dependencies..."
sudo apt install -y python3-pip python3-venv git chromium-browser

# Install additional packages for touchscreen
echo "🖥️ Installing touchscreen packages..."
sudo apt install -y xinput xrandr

# Enable UART
echo "🔌 Enabling UART..."
sudo raspi-config nonint do_serial 0

# Add user to dialout group
echo "👤 Adding user to dialout group..."
sudo usermod -a -G dialout $USER

# Create virtual environment
echo "🔧 Creating virtual environment..."
cd pi_zero
python3 -m venv venv
source venv/bin/activate

# Install Python packages
echo "📚 Installing Python packages..."
pip install -r requirements.txt

# Set permissions
echo "🔐 Setting permissions..."
chmod +x *.py

# Go back to project root
cd ..

# Create desktop shortcut
echo "🖥️ Creating desktop shortcut..."
mkdir -p ~/Desktop
cat > ~/Desktop/winding-controller.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Winding Controller
Comment=Pi Zero Winding Controller
Exec=chromium-browser --kiosk --disable-infobars http://localhost:5000
Icon=applications-electronics
Terminal=false
Categories=Electronics;
EOF

chmod +x ~/Desktop/winding-controller.desktop

# Create systemd service for auto-start
echo "🚀 Creating systemd service..."
PROJECT_DIR=$(pwd)
sudo tee /etc/systemd/system/winding-controller.service > /dev/null << EOF
[Unit]
Description=Pi CM4 Winding Controller
After=graphical.target

[Service]
Type=simple
User=pi
WorkingDirectory=${PROJECT_DIR}/pi_zero
Environment=PATH=${PROJECT_DIR}/pi_zero/venv/bin
Environment=DISPLAY=:0
ExecStart=${PROJECT_DIR}/pi_zero/venv/bin/python web_interface.py
Restart=always
RestartSec=10

[Install]
WantedBy=graphical.target
EOF

# Enable service
sudo systemctl daemon-reload
sudo systemctl enable winding-controller.service

echo "✅ Setup complete!"
echo ""
echo "🎮 Usage:"
echo "  Web Interface: python web_interface.py"
echo "  Simple Control: python simple_control.py"
echo "  Test UART: python test_uart.py"
echo "  Desktop Shortcut: Double-click 'Winding Controller'"
echo ""
echo "📱 Touchscreen interface will be available at: http://localhost:5000"
echo "🔌 Make sure Pico is connected via UART"
echo ""
echo "🔄 Reboot recommended to enable UART changes"
echo "   sudo reboot"
