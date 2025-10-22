#!/bin/bash
# Development environment setup script

echo "🚀 Setting up Pi Zero SKR Pico PUWinder development environment..."

# Update package lists
sudo apt-get update

# Install additional development tools
sudo apt-get install -y \
    cmake \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    build-essential \
    git \
    curl \
    wget \
    vim \
    nano \
    htop \
    tree \
    jq \
    unzip

# Install Python development tools
pip install --upgrade pip
pip install -r pi_zero/requirements.txt

# Install additional Python packages for development
pip install \
    ipython \
    jupyter \
    notebook \
    matplotlib \
    numpy \
    pandas \
    requests \
    beautifulsoup4 \
    lxml

# Set up Git configuration
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Create useful aliases
echo "alias ll='ls -la'" >> ~/.bashrc
echo "alias la='ls -A'" >> ~/.bashrc
echo "alias l='ls -CF'" >> ~/.bashrc
echo "alias ..='cd ..'" >> ~/.bashrc
echo "alias ...='cd ../..'" >> ~/.bashrc
echo "alias grep='grep --color=auto'" >> ~/.bashrc
echo "alias fgrep='fgrep --color=auto'" >> ~/.bashrc
echo "alias egrep='egrep --color=auto'" >> ~/.bashrc

# Set up project-specific aliases
echo "alias test-uart='cd pi_zero && python test_uart.py'" >> ~/.bashrc
echo "alias test-spindle='cd pi_zero && python test_spindle.py'" >> ~/.bashrc
echo "alias test-gcode='cd pi_zero && python test_gcode_interface.py'" >> ~/.bashrc
echo "alias build-pico='cd pico_firmware && mkdir -p build && cd build && cmake .. && make'" >> ~/.bashrc
echo "alias clean-build='cd pico_firmware && rm -rf build && mkdir build && cd build && cmake .. && make'" >> ~/.bashrc

# Create development directories
mkdir -p ~/dev/{logs,temp,backup}

# Set up logging
echo "export PYTHONPATH=\$PYTHONPATH:$(pwd)/pi_zero" >> ~/.bashrc

# Install GitHub CLI if not already installed
if ! command -v gh &> /dev/null; then
    curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list > /dev/null
    sudo apt-get update
    sudo apt-get install gh
fi

# Set up GitHub CLI authentication
echo "To complete setup, run: gh auth login"

# Create a welcome message
cat > ~/welcome.txt << 'EOF'
🎉 Pi Zero SKR Pico PUWinder Development Environment Ready!

Available commands:
- test-uart: Test UART communication
- test-spindle: Test spindle control
- test-gcode: Test G-code interface
- build-pico: Build Pico firmware
- clean-build: Clean build Pico firmware

Quick start:
1. cd pi_zero
2. python test_uart.py
3. python test_spindle.py

For help:
- Run: python -m pytest test_*.py -v
- Check: .github/workflows/ for CI/CD
- Read: DEVELOPMENT.md for detailed guide

Happy coding! 🚀
EOF

echo "✅ Development environment setup complete!"
echo "📖 Read ~/welcome.txt for quick start guide"
echo "🔧 Run 'source ~/.bashrc' to reload aliases"