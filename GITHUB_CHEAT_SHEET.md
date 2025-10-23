# GitHub CLI Cheat Sheet

## **Basic Commands You'll Need:**

### **Check Your Status:**
```bash
git status                    # See what's changed
git branch -a                 # See all branches (local and remote)
```

### **Create and Switch Branches:**
```bash
git checkout -b new-branch-name    # Create new branch
git checkout main                  # Switch to main
git checkout feature/your-branch   # Switch to your feature branch
```

### **Save Your Work:**
```bash
git add -A                       # Add all changes
git commit -m "Your message"      # Save changes
git push origin your-branch-name # Upload to GitHub
```

### **Fix Branch Protection Issues:**

#### **Option 1: Temporarily Disable Protection (EASIEST)**
1. Go to **GitHub.com → Your Repository → Settings → Branches**
2. Find your branch protection rule (usually for `main`)
3. Click **"Edit"**
4. **Uncheck "Require pull request reviews before merging"**
5. Click **"Save changes"**
6. Now you can merge your PR normally
7. **Re-enable protection** if you want

#### **Option 2: Use GitHub CLI (When Protection is On)**
```bash
# List your pull requests
gh pr list

# Merge a specific PR (replace 10 with your PR number)
gh pr merge 10 --admin --merge --delete-branch

# Create a new PR
gh pr create --title "Your Title" --body "Description"
```

### **Emergency Recovery (If You Lose Your Branch):**
```bash
# Get your branch back from remote
git checkout -b your-branch-name origin/your-branch-name

# Or get the latest from main
git checkout main
git pull origin main
```

### **Test Before Merging:**
```bash
# Test Pico compilation
cd pico_firmware
mkdir -p build && cd build
cmake .. && make -j4

# Test Python syntax
cd ../../pi_zero
python3 -m py_compile main_controller.py
python3 -m py_compile uart_api.py
```

## **🚨 EMERGENCY PROCEDURE (If Branch Protection Blocks You):**

### **Step 1: Check What's Happening**
```bash
git status
git branch -a
gh pr list
```

### **Step 2: Fix the Issue**
```bash
# If you're on the wrong branch:
git checkout your-branch-name

# If your branch is missing:
git checkout -b your-branch-name origin/your-branch-name
```

### **Step 3: Merge with Admin Override**
```bash
# Find your PR number first
gh pr list

# Then merge it (replace X with your PR number)
gh pr merge X --admin --merge --delete-branch
```

## **💡 Pro Tips:**

1. **Always test before merging:**
   ```bash
   cd pico_firmware/build && make -j4
   cd ../../pi_zero && python3 -m py_compile *.py
   ```

2. **Save this cheat sheet** - Bookmark this file!

3. **Use Option 1 (disable protection)** - It's the easiest and safest

4. **If you get stuck:** Just disable branch protection temporarily, merge, then re-enable it

## **🔧 Quick Fix Commands:**

```bash
# If you're lost, start here:
git status
git branch -a

# If you need to get back to your branch:
git checkout feature/advanced-spindle-control

# If you need to merge with admin override:
gh pr merge [PR_NUMBER] --admin --merge --delete-branch
```
