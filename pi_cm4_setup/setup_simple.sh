#!/bin/bash
# Simple Pi CM4 Setup Script

echo "🔄 Pi CM4 Simple Setup"
echo "======================"

# Get current directory
PROJECT_DIR=$(pwd)
echo "📁 Project directory: $PROJECT_DIR"

# Update system
echo "📦 Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install required packages
echo "🐍 Installing Python dependencies..."
sudo apt install -y python3-pip python3-venv git chromium-browser xinput xrandr

# Enable UART
echo "🔌 Enabling UART..."
sudo raspi-config nonint do_serial 0

# Add user to dialout group
echo "👤 Adding user to dialout group..."
sudo usermod -a -G dialout $USER

# Create pi_zero directory if it doesn't exist
echo "📁 Creating pi_zero directory..."
mkdir -p pi_zero

# Create virtual environment
echo "🔧 Creating virtual environment..."
cd pi_zero
python3 -m venv venv
source venv/bin/activate

# Install Python packages
echo "📚 Installing Python packages..."
pip install -r requirements.txt

# Set permissions for Python files
echo "🔐 Setting permissions..."
find . -name "*.py" -exec chmod +x {} \;

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

echo "✅ Setup complete!"
echo ""
echo "🎮 Usage:"
echo "  cd pi_zero"
echo "  python test_uart.py"
echo "  python web_interface.py"
echo "  python simple_control.py"
echo ""
echo "📱 Touchscreen interface: http://localhost:5000"
echo "🔌 Make sure Pico is connected via UART"
echo ""
echo "🔄 Reboot recommended: sudo reboot"
