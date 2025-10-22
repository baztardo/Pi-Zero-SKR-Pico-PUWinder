#!/bin/bash
# AI Documentation Generator Script
# Automatically generates and updates documentation using AI

set -e  # Exit on any error

echo "🤖 AI Documentation Generator"
echo "=============================="

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is required but not installed"
    exit 1
fi

# Check if we're in the right directory
if [ ! -f "scripts/ai_docs_generator.py" ]; then
    echo "❌ Please run this script from the project root directory"
    exit 1
fi

# Create docs directory if it doesn't exist
mkdir -p docs

echo "📝 Generating AI documentation..."

# Run the AI documentation generator
python3 scripts/ai_docs_generator.py

echo ""
echo "✅ AI Documentation Generation Complete!"
echo ""
echo "📚 Generated files:"
ls -la docs/*.md 2>/dev/null || echo "No documentation files found"

echo ""
echo "🔗 View documentation:"
echo "  - Project Overview: docs/project_overview.md"
echo "  - Setup Guide: docs/setup_guide.md"
echo "  - Troubleshooting: docs/troubleshooting_guide.md"
echo "  - Python API: docs/python_api.md"
echo "  - C++ API: docs/cpp_api.md"

echo ""
echo "📊 Documentation statistics:"
for file in docs/*.md; do
    if [ -f "$file" ]; then
        lines=$(wc -l < "$file")
        size=$(du -h "$file" | cut -f1)
        filename=$(basename "$file")
        echo "  - $filename: $lines lines, $size"
    fi
done

echo ""
echo "🚀 Next steps:"
echo "  1. Review the generated documentation"
echo "  2. Commit changes: git add docs/ && git commit -m 'docs: update AI-generated documentation'"
echo "  3. Push to GitHub: git push origin main"
echo "  4. Documentation will be automatically deployed to GitHub Pages"

echo ""
echo "🎉 Documentation generation complete!"
