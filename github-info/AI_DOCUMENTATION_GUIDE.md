# 🤖 AI Documentation Guide

## 📋 Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [AI Documentation Features](#ai-documentation-features)
4. [Automated Workflow](#automated-workflow)
5. [Customization](#customization)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)

## 🎯 Overview

The AI Documentation Generator automatically creates and maintains comprehensive documentation for your Pi Zero SKR Pico PUWinder project. It analyzes your codebase and generates:

- **Project Overview** - Complete system architecture and features
- **Setup Guide** - Step-by-step installation instructions
- **API Documentation** - Python and C++ API references
- **Troubleshooting Guide** - Common issues and solutions
- **README** - Project introduction and quick start

## 🚀 Quick Start

### **1. Generate Documentation Locally**
```bash
# Run the AI documentation generator
./scripts/generate_docs.sh

# Or run directly with Python
python3 scripts/ai_docs_generator.py
```

### **2. View Generated Documentation**
```bash
# Open documentation in your browser
open docs/project_overview.md
open docs/setup_guide.md
open docs/troubleshooting_guide.md
```

### **3. Commit and Push**
```bash
# Add documentation to git
git add docs/
git commit -m "docs: update AI-generated documentation"
git push origin main
```

## 🤖 AI Documentation Features

### **1. Automatic Code Analysis**
- **Python Files**: Extracts functions, classes, and docstrings
- **C++ Files**: Analyzes headers and implementation files
- **Dependencies**: Identifies required packages and libraries
- **Structure**: Understands project organization

### **2. Intelligent Content Generation**
- **Context-Aware**: Generates content based on your specific codebase
- **Comprehensive**: Covers all aspects of the project
- **Accurate**: Reflects actual implementation details
- **Up-to-Date**: Automatically updates when code changes

### **3. Professional Formatting**
- **Markdown**: Clean, readable documentation
- **Structure**: Organized with clear headings and sections
- **Links**: Cross-references between documents
- **Badges**: Status indicators and version information

## 🔄 Automated Workflow

### **GitHub Actions Integration**
The documentation is automatically generated and updated:

#### **Triggers:**
- **Push to main/develop** - Updates documentation on code changes
- **Pull Requests** - Generates docs for review
- **Daily Schedule** - Ensures docs stay current
- **Manual Trigger** - On-demand generation

#### **Process:**
1. **Code Analysis** - Scans Python and C++ files
2. **Content Generation** - Creates documentation using AI
3. **Quality Check** - Validates links and formatting
4. **Deployment** - Publishes to GitHub Pages
5. **Notification** - Updates project status

### **Generated Files:**
```
docs/
├── project_overview.md      # System architecture and features
├── setup_guide.md          # Installation and configuration
├── troubleshooting_guide.md # Common issues and solutions
├── python_api.md          # Python API reference
├── cpp_api.md            # C++ API reference
└── index.html            # Documentation homepage
```

## 🛠️ Customization

### **1. Modify AI Prompts**
Edit `scripts/ai_docs_generator.py` to customize:

```python
# Customize project description
overview = f"""# {project_name} - Project Overview

## 🤖 AI-Generated Documentation
This documentation is automatically generated using AI...

## 🎯 Project Purpose
Your custom project description here...
"""
```

### **2. Add Custom Sections**
```python
def generate_custom_section(self):
    """Generate custom documentation section"""
    custom_docs = f"""# Custom Section
    
## Your Custom Content
Add your specific documentation here...
"""
    
    with open(self.docs_dir / "custom_section.md", 'w') as f:
        f.write(custom_docs)
```

### **3. Include Additional Files**
```python
# Add more file patterns to analyze
python_files = list(self.pi_zero_dir.glob("*.py"))
python_files.extend(list(self.pi_zero_dir.glob("utils/*.py")))
python_files.extend(list(self.pi_zero_dir.glob("tests/*.py")))
```

## 📚 Best Practices

### **1. Code Documentation**
Write good docstrings for better AI analysis:

```python
def calculate_traverse_speed(spindle_rpm: float, wire_diameter: float) -> float:
    """
    Calculate traverse speed based on spindle RPM and wire diameter.
    
    Args:
        spindle_rpm: Spindle speed in RPM
        wire_diameter: Wire diameter in mm
        
    Returns:
        Traverse speed in mm/min
        
    Raises:
        ValueError: If inputs are negative
    """
    if spindle_rpm < 0 or wire_diameter < 0:
        raise ValueError("Inputs must be positive")
    
    return (spindle_rpm * wire_diameter * math.pi) / 1000.0
```

### **2. Type Hints**
Use type hints for better AI understanding:

```python
from typing import List, Dict, Optional

def process_gcode_commands(commands: List[str]) -> Dict[str, bool]:
    """Process a list of G-code commands"""
    results = {}
    for command in commands:
        results[command] = execute_command(command)
    return results
```

### **3. Meaningful Comments**
Add comments that explain the "why", not just the "what":

```python
# Calculate PWM duty cycle for 16-bit resolution
# This ensures smooth motor control without stepping
duty_cycle = (rpm / MAX_RPM) * 65535.0
```

### **4. Consistent Naming**
Use consistent naming conventions:

```python
# Good: Clear, descriptive names
def set_spindle_rpm(target_rpm: float) -> bool:
    """Set spindle RPM with validation"""
    pass

# Avoid: Unclear abbreviations
def set_spd_rpm(rpm: float) -> bool:
    pass
```

## 🔧 Troubleshooting

### **Common Issues**

#### **1. Python Import Errors**
```bash
# Error: ModuleNotFoundError: No module named 'ast'
# Solution: Install required packages
pip install ast pyyaml
```

#### **2. Permission Denied**
```bash
# Error: Permission denied: scripts/generate_docs.sh
# Solution: Make script executable
chmod +x scripts/generate_docs.sh
```

#### **3. Documentation Not Updating**
```bash
# Check if files are being analyzed
python3 scripts/ai_docs_generator.py --verbose

# Check file permissions
ls -la docs/
```

#### **4. GitHub Actions Failing**
```bash
# Check workflow logs
# Go to: GitHub → Actions → Failed workflow
# Look for specific error messages
```

### **Debug Commands**

#### **Test Documentation Generation**
```bash
# Run with verbose output
python3 scripts/ai_docs_generator.py --project-root . --output-dir docs

# Check generated files
ls -la docs/
```

#### **Validate Documentation**
```bash
# Check markdown syntax
find docs -name "*.md" -exec markdownlint {} \;

# Check for broken links
find docs -name "*.md" -exec markdown-link-check {} \;
```

#### **Test GitHub Actions Locally**
```bash
# Install act (GitHub Actions runner)
# https://github.com/nektos/act

# Run documentation workflow
act -j generate-docs
```

## 📊 Documentation Statistics

### **Generated Content**
- **Project Overview**: ~2000 words
- **Setup Guide**: ~1500 words
- **API Documentation**: ~1000 words per API
- **Troubleshooting**: ~1000 words
- **README**: ~800 words

### **Update Frequency**
- **Automatic**: On every code change
- **Scheduled**: Daily at 2 AM UTC
- **Manual**: On-demand via workflow dispatch

### **Quality Metrics**
- **Link Validation**: All internal links checked
- **Markdown Syntax**: Validated for correctness
- **File Size**: Optimized for readability
- **Structure**: Consistent formatting

## 🚀 Advanced Features

### **1. Custom AI Prompts**
```python
# Add custom AI prompts for specific sections
def generate_advanced_troubleshooting(self):
    """Generate advanced troubleshooting using custom AI prompts"""
    prompt = """
    Analyze the following codebase and generate comprehensive 
    troubleshooting documentation for a precision winding machine:
    
    Focus on:
    - Hardware-specific issues
    - Software debugging techniques
    - Performance optimization
    - Safety considerations
    """
    # Implement custom AI logic here
```

### **2. Multi-Language Support**
```python
# Generate documentation in multiple languages
def generate_multilingual_docs(self):
    """Generate documentation in multiple languages"""
    languages = ['en', 'es', 'fr', 'de']
    for lang in languages:
        # Generate localized documentation
        pass
```

### **3. Interactive Documentation**
```python
# Generate interactive examples
def generate_interactive_examples(self):
    """Generate interactive code examples"""
    examples = """
    ```python
    # Interactive example - try this code
    from pi_zero.main_controller import WindingController
    
    controller = WindingController()
    controller.start_winding(rpm=1000, wire_diameter=0.5)
    ```
    """
```

## 🎯 Next Steps

### **1. Immediate Actions**
- Run `./scripts/generate_docs.sh` to generate initial documentation
- Review generated files in `docs/` directory
- Commit and push to GitHub

### **2. Customization**
- Edit `scripts/ai_docs_generator.py` for custom content
- Add your specific project details
- Include additional file patterns

### **3. Automation**
- Enable GitHub Actions for automatic updates
- Set up GitHub Pages for documentation hosting
- Configure notifications for documentation changes

### **4. Advanced Features**
- Add custom AI prompts for specific sections
- Implement multi-language support
- Create interactive documentation examples

---

**🎉 Your AI-powered documentation system is ready!**

The AI Documentation Generator will automatically create and maintain comprehensive documentation for your project, ensuring it stays up-to-date with your codebase.
