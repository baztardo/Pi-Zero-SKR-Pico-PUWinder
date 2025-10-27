#!/bin/bash
# Working Pi CM4 Setup Script

echo "🔄 Pi CM4 Working Setup"
echo "======================="

# Check if we're in the right directory
if [ ! -d "pi_zero" ]; then
    echo "❌ pi_zero directory not found!"
    echo "   Make sure you're in the project root directory"
    echo "   Current directory: $(pwd)"
    exit 1
fi

# Update system
echo "📦 Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install required packages
echo "🐍 Installing Python dependencies..."
sudo apt install -y python3-pip python3-venv git

# Try to install a browser (use what's available)
echo "🌐 Installing browser..."
if apt list --installed | grep -q chromium-browser; then
    echo "   Chromium already installed"
elif apt list --installed | grep -q firefox; then
    echo "   Firefox already installed"
else
    echo "   Installing Firefox..."
    sudo apt install -y firefox-esr || sudo apt install -y firefox || echo "   No browser installed - you can use any web browser"
fi

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
pip install flask pyserial

# Set permissions for Python files
echo "🔐 Setting permissions..."
for file in *.py; do
    if [ -f "$file" ]; then
        chmod +x "$file"
        echo "   ✅ Made $file executable"
    fi
done

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
