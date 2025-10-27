#!/bin/bash
# Minimal Pi CM4 Setup Script

echo "🔄 Pi CM4 Minimal Setup"
echo "======================="

# Update system
echo "📦 Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install required packages
echo "🐍 Installing Python dependencies..."
sudo apt install -y python3-pip python3-venv git chromium-browser

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
if [ -f "requirements.txt" ]; then
    pip install -r requirements.txt
else
    echo "⚠️  requirements.txt not found, installing basic packages..."
    pip install flask pyserial
fi

# Set permissions
echo "🔐 Setting permissions..."
chmod +x *.py

echo "✅ Setup complete!"
echo ""
echo "🎮 Usage:"
echo "  cd pi_zero"
echo "  source venv/bin/activate"
echo "  python test_uart.py"
echo "  python web_interface.py"
echo "  python simple_control.py"
echo ""
echo "📱 Web interface: http://localhost:5000"
echo "🔌 Make sure Pico is connected via UART"
echo ""
echo "🔄 Reboot recommended: sudo reboot"
