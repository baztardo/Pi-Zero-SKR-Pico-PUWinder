# Development Guide

## 🚀 Getting Started with GitHub Automation

This guide shows you how to leverage GitHub's powerful automation features to improve your development workflow.

## 📋 Table of Contents

1. [GitHub Actions CI/CD](#github-actions-cicd)
2. [Cursor Integration](#cursor-integration)
3. [Automated Testing](#automated-testing)
4. [Code Quality Automation](#code-quality-automation)
5. [Debugging with GitHub](#debugging-with-github)
6. [Release Automation](#release-automation)
7. [Development Workflow](#development-workflow)

## 🔄 GitHub Actions CI/CD

### **Automated Build & Test Pipeline**

Our GitHub Actions automatically:

- ✅ **Builds Pico firmware** on every push
- ✅ **Tests Python code** with pytest
- ✅ **Runs code quality checks** (linting, formatting)
- ✅ **Security scanning** with Trivy
- ✅ **Generates documentation** automatically

### **Triggering Workflows**

```bash
# Push to main/develop branches
git push origin main

# Create a pull request
git checkout -b feature/new-feature
git push origin feature/new-feature
# Then create PR on GitHub

# Manual workflow dispatch
# Go to Actions tab → Debug and Development Tools → Run workflow
```

### **Workflow Types**

1. **CI Pipeline** (`.github/workflows/ci.yml`)
   - Runs on every push/PR
   - Builds firmware
   - Tests Python code
   - Code quality checks

2. **Debug Pipeline** (`.github/workflows/debug.yml`)
   - Manual trigger only
   - Memory debugging
   - Performance profiling
   - Static analysis

3. **Release Pipeline** (`.github/workflows/release.yml`)
   - Runs on version tags
   - Creates release packages
   - Generates changelogs

## 🎯 Cursor Integration

### **VS Code Configuration**

The `.vscode/` folder provides:

- **Debug configurations** for Python code
- **Build tasks** for Pico firmware
- **Code formatting** with Black and clang-format
- **IntelliSense** for C++ and Python

### **Using Cursor's AI Features**

1. **Code Completion**: Cursor's AI helps with:
   - G-code command implementation
   - Hardware interface code
   - Error handling patterns

2. **Code Review**: Ask Cursor to:
   - Review your code for bugs
   - Suggest improvements
   - Explain complex logic

3. **Refactoring**: Use Cursor to:
   - Extract functions
   - Rename variables consistently
   - Optimize performance

### **Debugging with Cursor**

```bash
# Set breakpoints in VS Code
# Use F5 to start debugging
# Or use the debug configurations in .vscode/launch.json
```

## 🧪 Automated Testing

### **Python Testing**

```bash
# Run all tests
cd pi_zero
python -m pytest test_*.py -v

# Run with coverage
python -m pytest test_*.py --cov=. --cov-report=html

# Run specific test
python -m pytest test_uart.py::test_ping_command -v
```

### **Firmware Testing**

```bash
# Build and test firmware
cd pico_firmware
mkdir build && cd build
cmake .. && make

# Run static analysis
cppcheck src/
```

### **Integration Testing**

```bash
# Test full system (simulated)
cd pi_zero
python test_klipper_architecture.py --simulate
```

## 🔍 Code Quality Automation

### **Automated Code Formatting**

```bash
# Format Python code
cd pi_zero
black .

# Format C++ code
cd pico_firmware
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### **Linting & Static Analysis**

```bash
# Python linting
cd pi_zero
flake8 .
mypy .

# C++ static analysis
cd pico_firmware
cppcheck src/
```

### **Security Scanning**

GitHub Actions automatically runs:
- **Bandit** for Python security issues
- **Safety** for dependency vulnerabilities
- **Trivy** for container/OS vulnerabilities

## 🐛 Debugging with GitHub

### **Using GitHub Issues**

1. **Bug Reports**: Use the bug report template
2. **Feature Requests**: Use the feature request template
3. **Discussions**: Use GitHub Discussions for questions

### **Debug Workflows**

```bash
# Trigger debug workflow
# Go to Actions → Debug and Development Tools
# Select debug type:
# - Memory debugging
# - Performance profiling
# - Static analysis
# - Runtime debugging
```

### **Code Review Process**

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/new-feature
   ```

2. **Make Changes & Test**
   ```bash
   # Test locally
   python -m pytest test_*.py
   cd pico_firmware && make
   ```

3. **Push & Create PR**
   ```bash
   git push origin feature/new-feature
   # Create PR on GitHub
   ```

4. **Automated Checks**
   - CI pipeline runs automatically
   - Code quality checks
   - Security scanning
   - Documentation generation

## 📦 Release Automation

### **Creating Releases**

```bash
# Create a version tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# GitHub Actions automatically:
# - Builds release package
# - Creates GitHub release
# - Generates changelog
```

### **Release Artifacts**

Each release includes:
- ✅ Compiled Pico firmware (`.uf2` file)
- ✅ Pi Zero Python code
- ✅ Documentation
- ✅ Installation instructions

## 🔄 Development Workflow

### **Daily Development**

1. **Start Development**
   ```bash
   git checkout develop
   git pull origin develop
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature
   ```

3. **Develop with Cursor**
   - Use Cursor's AI for code completion
   - Set breakpoints for debugging
   - Use tasks for building/testing

4. **Test Locally**
   ```bash
   # Test Python code
   cd pi_zero && python -m pytest
   
   # Build firmware
   cd pico_firmware && make
   ```

5. **Commit & Push**
   ```bash
   git add .
   git commit -m "feat: add new feature"
   git push origin feature/your-feature
   ```

6. **Create Pull Request**
   - GitHub automatically runs CI
   - Review code with team
   - Merge when approved

### **Weekly Maintenance**

1. **Update Dependencies**
   ```bash
   # Dependabot creates PRs automatically
   # Review and merge dependency updates
   ```

2. **Run Debug Workflows**
   - Memory profiling
   - Performance analysis
   - Security scanning

3. **Update Documentation**
   - GitHub Actions generates docs automatically
   - Review and update as needed

## 🛠️ Advanced GitHub Features

### **GitHub Copilot Integration**

```python
# Cursor can suggest code like this:
def process_gcode_command(command: str) -> bool:
    """Process G-code command with error handling"""
    try:
        # Cursor suggests implementation
        return gcode_interface.execute_command(command)
    except Exception as e:
        logger.error(f"G-code error: {e}")
        return False
```

### **Automated Documentation**

- **Doxygen** for C++ documentation
- **Sphinx** for Python documentation
- **GitHub Pages** for hosting
- **Auto-generated** on every push

### **Dependency Management**

```yaml
# .github/dependabot.yml automatically:
# - Updates Python dependencies
# - Updates GitHub Actions
# - Creates PRs for updates
```

## 🎯 Best Practices

### **Code Quality**

1. **Write Tests First**
   ```python
   def test_spindle_control():
       # Test implementation
       assert spindle.get_rpm() == expected_rpm
   ```

2. **Use Type Hints**
   ```python
   def set_spindle_speed(rpm: float) -> bool:
       """Set spindle speed with type safety"""
   ```

3. **Document Everything**
   ```cpp
   /**
    * @brief Set spindle speed in RPM
    * @param rpm Target RPM (0-3000)
    * @return true if successful
    */
   bool set_spindle_speed(float rpm);
   ```

### **Git Workflow**

1. **Use Conventional Commits**
   ```bash
   git commit -m "feat: add spindle control"
   git commit -m "fix: resolve UART communication issue"
   git commit -m "docs: update installation guide"
   ```

2. **Keep Commits Small**
   - One feature per commit
   - One bug fix per commit
   - Clear commit messages

3. **Use Pull Requests**
   - Always use PRs for code review
   - Use templates for consistency
   - Link issues to PRs

## 🚀 Getting Started

1. **Fork the Repository**
   - Click "Fork" on GitHub
   - Clone your fork locally

2. **Set Up Development Environment**
   ```bash
   # Install Python dependencies
   cd pi_zero
   pip install -r requirements.txt
   
   # Set up Pico SDK
   # Follow Pico SDK installation guide
   ```

3. **Enable GitHub Actions**
   - Go to repository Settings
   - Enable Actions
   - Configure secrets if needed

4. **Start Developing**
   - Create feature branch
   - Make changes
   - Test locally
   - Push and create PR

## 📚 Additional Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Cursor Documentation](https://cursor.sh/docs)
- [Pico SDK Documentation](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf)
- [Python Testing Guide](https://docs.python.org/3/library/unittest.html)

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Create a pull request
6. Respond to feedback
7. Merge when approved

---

**Happy Coding! 🎉**

This setup gives you a professional development environment with automated testing, code quality checks, and seamless integration between Cursor and GitHub.
