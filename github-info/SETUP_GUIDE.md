# 🚀 Complete GitHub Setup Guide

## 📋 Table of Contents

1. [Creating Issues with Templates](#creating-issues-with-templates)
2. [Setting Up Branch Protection](#setting-up-branch-protection)
3. [GitHub Copilot Integration](#github-copilot-integration)
4. [GitHub Codespaces Setup](#github-codespaces-setup)
5. [Advanced AI Features](#advanced-ai-features)

## 🎯 Creating Issues with Templates

### **Step 1: Go to Your Repository**
```
https://github.com/your-username/Pi-Zero-SKR-Pico-PUWinder/issues
```

### **Step 2: Create New Issue**
- Click **"New Issue"** button
- You'll see these templates:
  - 🐛 **Bug Report** - For software bugs
  - ✨ **Feature Request** - For new features  
  - ⚡ **Performance Issue** - For performance problems
  - 🔧 **Hardware Issue** - For hardware problems

### **Step 3: Fill Out Template**
Each template guides you through:
- **Problem Description**
- **Steps to Reproduce**
- **Expected Behavior**
- **Hardware/Software Info**
- **Logs and Screenshots**

### **Step 4: Submit Issue**
- Add appropriate labels
- Assign to team members
- Link to project boards

## 🛡️ Setting Up Branch Protection

### **Step 1: Go to Repository Settings**
```
https://github.com/your-username/Pi-Zero-SKR-Pico-PUWinder/settings/branches
```

### **Step 2: Add Branch Protection Rule**
- Click **"Add rule"**
- Branch name pattern: `main`
- Enable these settings:

#### **Required Settings:**
```yaml
✅ Require a pull request before merging
  ✅ Require approvals (1)
  ✅ Dismiss stale PR approvals when new commits are pushed
  ✅ Require review from code owners

✅ Require status checks to pass before merging
  ✅ Require branches to be up to date before merging
  ✅ Status checks: "Build Pico Firmware", "Test Pi Zero", "Code Quality"

✅ Require conversation resolution before merging
✅ Require signed commits
✅ Require linear history
✅ Include administrators
✅ Restrict pushes that create files larger than 100MB
```

### **Step 3: Configure Code Owners**
The `.github/CODEOWNERS` file is already set up:
```
# Global owners
* @your-username

# Pi Zero Python code
/pi_zero/ @your-username

# Pico firmware C++ code
/pico_firmware/ @your-username

# Documentation
/docs/ @your-username
*.md @your-username
```

### **Step 4: Required Status Checks**
Configure these status checks:
- ✅ **Build Pico Firmware** - Must pass
- ✅ **Test Pi Zero** - Must pass  
- ✅ **Code Quality** - Must pass
- ✅ **Security Scan** - Must pass

## 🤖 GitHub Copilot Integration

### **Step 1: Enable GitHub Copilot**
1. Go to **GitHub Settings** → **Copilot**
2. Enable Copilot for your account
3. Install VS Code Copilot extension

### **Step 2: Configure Copilot for Your Project**

**Cursor/VS Code Settings:**
```json
{
  "github.copilot.enable": {
    "*": true,
    "yaml": false,
    "plaintext": false
  },
  "github.copilot.advanced": {
    "debug.overrideEngine": "copilot-codex"
  }
}
```

### **Step 3: Using Copilot Effectively**

#### **Code Completion Examples:**

**Python G-code Processing:**
```python
def process_gcode_command(self, command: str) -> bool:
    """Process G-code command with error handling"""
    try:
        # Copilot suggests:
        if command.startswith('M3'):
            return self._handle_spindle_start(command)
        elif command.startswith('M4'):
            return self._handle_spindle_reverse(command)
        elif command.startswith('M5'):
            return self._handle_spindle_stop()
        elif command.startswith('G1'):
            return self._handle_linear_move(command)
        else:
            return self._handle_unknown_command(command)
    except Exception as e:
        self.logger.error(f"G-code processing error: {e}")
        return False
```

**C++ Hardware Interface:**
```cpp
class SpindleController {
public:
    // Copilot suggests complete method
    bool setRPM(float rpm) {
        if (rpm < 0 || rpm > MAX_RPM) {
            return false;
        }
        
        // Calculate PWM duty cycle
        float duty_cycle = (rpm / MAX_RPM) * 100.0f;
        
        // Set PWM
        pwm_set_gpio_level(SPINDLE_PWM_PIN, duty_cycle);
        
        return true;
    }
};
```

#### **Copilot Chat Integration:**

**Ask Copilot:**
```
// Explain this G-code command: M3 S1000
// How do I implement trapezoidal velocity profiles?
// What's the best way to handle UART communication errors?
// Generate unit tests for this function
```

### **Step 4: Copilot Best Practices**

#### **Write Good Comments:**
```python
# Copilot understands context better with comments
def calculate_traverse_speed(spindle_rpm: float, wire_diameter: float) -> float:
    """
    Calculate traverse speed based on spindle RPM and wire diameter.
    
    Args:
        spindle_rpm: Spindle speed in RPM
        wire_diameter: Wire diameter in mm
        
    Returns:
        Traverse speed in mm/min
    """
    # Copilot will suggest the calculation
    return (spindle_rpm * wire_diameter * math.pi) / 1000.0
```

#### **Use Type Hints:**
```python
# Copilot works better with type hints
def process_command(command: str) -> bool:
    """Process G-code command"""
    # Copilot suggests implementation
    pass
```

## 🌐 GitHub Codespaces Setup

### **Step 1: Enable Codespaces**
1. Go to your repository
2. Click **"Code"** → **"Codespaces"**
3. Click **"Create codespace on main"**

### **Step 2: Configure Development Environment**

The `.devcontainer/devcontainer.json` is already configured:
```json
{
  "name": "Pi Zero SKR Pico PUWinder Development",
  "image": "mcr.microsoft.com/devcontainers/python:3.11",
  
  "features": {
    "ghcr.io/devcontainers/features/git:1": {},
    "ghcr.io/devcontainers/features/github-cli:1": {},
    "ghcr.io/devcontainers/features/docker-in-docker:2": {}
  },
  
  "customizations": {
    "vscode": {
      "extensions": [
        "ms-python.python",
        "ms-python.pylint",
        "ms-python.black-formatter",
        "github.copilot",
        "github.copilot-chat",
        "ms-vscode.cpptools",
        "ms-vscode.cmake-tools"
      ]
    }
  }
}
```

### **Step 3: Using Codespaces**

#### **Development Workflow:**
```bash
# Codespace automatically:
# - Installs Python 3.11
# - Installs dependencies
# - Sets up VS Code with extensions
# - Configures Copilot

# Start developing immediately:
cd pi_zero
python test_uart.py
```

#### **Port Forwarding:**
- **UART simulation**: Port 8080
- **Web interface**: Port 3000
- **Documentation**: Port 8000

### **Step 4: Codespace Benefits**
- ✅ **Instant Setup** - No local installation needed
- ✅ **Consistent Environment** - Same setup for all developers
- ✅ **Cloud Development** - Work from anywhere
- ✅ **AI Integration** - Copilot and Cursor work seamlessly

## 🤖 Advanced AI Features

### **1. GitHub Copilot Chat**

#### **Code Explanation:**
```
// Explain this function:
def sync_traverse_to_spindle(self):
    """Sync traverse movement to spindle RPM"""
    current_rpm = self.spindle_controller.get_rpm()
    if current_rpm > 0:
        traverse_speed = self.calculate_traverse_speed(current_rpm)
        self.traverse_controller.set_speed(traverse_speed)
```

#### **Code Generation:**
```
// Generate a function to handle M3 command (spindle start)
// Include error handling and logging
// Use type hints and docstrings
```

#### **Code Review:**
```
// Review this code for potential issues:
def process_uart_command(self, command):
    if command == "PING":
        return "PONG"
    elif command == "VERSION":
        return "1.0"
    else:
        return "UNKNOWN"
```

### **2. AI-Powered Development**

#### **Smart Code Completion:**
- Copilot suggests complete functions
- Context-aware suggestions
- Error handling patterns
- Test generation

#### **Intelligent Refactoring:**
- Extract functions
- Rename variables
- Optimize performance
- Improve readability

#### **Automated Documentation:**
- Generate docstrings
- Create API documentation
- Write README files
- Generate comments

### **3. GitHub Actions with AI**

#### **Smart CI/CD:**
```yaml
# .github/workflows/smart-ci.yml
name: Smart CI/CD
on: [push, pull_request]

jobs:
  smart-testing:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: AI Code Analysis
        uses: github/copilot-action@v1
        with:
          prompt: "Analyze this codebase for potential issues"
          
      - name: Smart Test Generation
        run: |
          # AI generates tests based on code changes
          python scripts/generate_tests.py
```

## 🔄 Complete Development Workflow

### **Step 1: Create Feature Branch**
```bash
# Use GitHub CLI
gh repo clone your-username/Pi-Zero-SKR-Pico-PUWinder
cd Pi-Zero-SKR-Pico-PUWinder

# Create feature branch
git checkout -b feature/advanced-spindle-control
```

### **Step 2: Develop with AI Assistance**
```bash
# Open in Cursor with Copilot
cursor .

# Use Copilot for:
# - Code completion
# - Function generation
# - Error handling
# - Test generation
```

### **Step 3: Test Locally**
```bash
# Run tests
cd pi_zero
python -m pytest test_*.py -v

# Build firmware
cd ../pico_firmware
mkdir build && cd build
cmake .. && make
```

### **Step 4: Create Pull Request**
```bash
# Push changes
git add .
git commit -m "feat: add advanced spindle control"
git push origin feature/advanced-spindle-control

# Create PR with template
gh pr create --title "Advanced Spindle Control" --body "Implements advanced spindle control features"
```

### **Step 5: Automated Review**
- GitHub Actions run automatically
- Copilot suggests improvements
- Code quality checks pass
- Security scan completes

### **Step 6: Merge and Release**
```bash
# After approval, merge
git checkout main
git pull origin main

# Create release
git tag -a v1.1.0 -m "Release v1.1.0"
git push origin v1.1.0
```

## 🎯 Best Practices Summary

### **1. Issue Management**
- ✅ Use templates for consistent reporting
- ✅ Label issues appropriately
- ✅ Assign to team members
- ✅ Link to project boards

### **2. Branch Protection**
- ✅ Require PR reviews
- ✅ Require status checks
- ✅ Require linear history
- ✅ Protect main branch

### **3. AI Integration**
- ✅ Use Copilot for code completion
- ✅ Use Copilot Chat for explanations
- ✅ Write good comments for context
- ✅ Use type hints for better suggestions

### **4. Development Workflow**
- ✅ Create feature branches
- ✅ Test locally before pushing
- ✅ Use PR templates
- ✅ Automate everything possible

## 🚀 Quick Start Checklist

### **Immediate Setup:**
- [ ] Go to repository Settings → Branches
- [ ] Add branch protection rule for `main`
- [ ] Enable required status checks
- [ ] Set up code owners

### **Issue Management:**
- [ ] Test issue templates
- [ ] Create sample issues
- [ ] Set up labels and milestones
- [ ] Configure project boards

### **AI Integration:**
- [ ] Enable GitHub Copilot
- [ ] Install VS Code extensions
- [ ] Test code completion
- [ ] Try Copilot Chat

### **Development Environment:**
- [ ] Test Codespaces
- [ ] Verify port forwarding
- [ ] Run test suite
- [ ] Build firmware

---

**🎉 You now have a professional, AI-powered development environment!**

This setup gives you:
- **Automated Issue Management** with templates
- **Protected Branches** with required reviews
- **AI-Powered Development** with Copilot
- **Cloud Development** with Codespaces
- **Smart Automation** with GitHub Actions

Your development workflow is now enterprise-grade with AI assistance at every step!
