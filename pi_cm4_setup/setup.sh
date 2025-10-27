#!/bin/bash
# Working Pi CM4 Setup Script

echo "🔄 Pi CM4 Working Setup"
echo "======================="

# Find the project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "📁 Script location: $SCRIPT_DIR"
echo "📁 Project root: $PROJECT_ROOT"

# Check if pi_zero directory exists
if [ ! -d "$PROJECT_ROOT/pi_zero" ]; then
    echo "❌ pi_zero directory not found!"
    echo "   Expected at: $PROJECT_ROOT/pi_zero"
    echo "   Current structure:"
    ls -la "$PROJECT_ROOT" | grep -E "(pi_zero|git)"
    exit 1
fi

# Change to project root
cd "$PROJECT_ROOT"
echo "📁 Working from: $(pwd)"
echo "📁 pi_zero path: $(pwd)/pi_zero"

# Update system
echo "📦 Updating system packages..."
sudo apt-get update && sudo apt-get upgrade -y

# Install required packages
echo "🐍 Installing Python dependencies..."
sudo apt-get install -y python3-pip python3-venv git

# Try to install a browser (use what's available)
echo "🌐 Installing browser..."
if dpkg -l | grep -q chromium-browser; then
    echo "   Chromium already installed"
elif dpkg -l | grep -q firefox; then
    echo "   Firefox already installed"
else
    echo "   Installing Firefox..."
    sudo apt-get install -y firefox-esr || sudo apt-get install -y firefox || echo "   No browser installed - you can use any web browser"
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
