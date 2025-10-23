# GitHub Issues & Pull Requests Guide

## **🔍 Understanding the Difference:**

### **Issues** = Bug reports, feature requests, discussions
- **Purpose:** Report problems, request features, discuss ideas
- **No code changes** - just text descriptions
- **Anyone can create** - no approval needed
- **You can close them** when resolved

### **Pull Requests** = Code changes that need approval
- **Purpose:** Submit code changes for review
- **Contains actual code** - files changed
- **Need approval** to merge into main branch
- **Branch protection** can block you from approving your own

## **📋 GitHub Issues Commands:**

### **List Issues:**
```bash
gh issue list                    # List all issues
gh issue list --state open       # List only open issues
gh issue list --state closed     # List only closed issues
```

### **Create Issues:**
```bash
# Create a new issue
gh issue create --title "Bug: UART not working" --body "Description of the problem"

# Create with labels
gh issue create --title "Feature: Add LCD support" --body "Need LCD display" --label "enhancement"
```

### **Manage Issues:**
```bash
gh issue view 1                   # View issue #1
gh issue close 1                  # Close issue #1
gh issue reopen 1                 # Reopen issue #1
gh issue comment 1 --body "Fixed!" # Add comment to issue #1
```

### **Assign Issues:**
```bash
gh issue edit 1 --add-assignee @me    # Assign to yourself
gh issue edit 1 --remove-assignee @me # Remove assignment
```

## **🔧 Pull Request Commands:**

### **List Pull Requests:**
```bash
gh pr list                    # List all PRs
gh pr list --state open       # List open PRs
gh pr list --state merged     # List merged PRs
```

### **Create Pull Requests:**
```bash
# Create PR from current branch
gh pr create --title "Fix compilation errors" --body "Description of changes"

# Create PR with specific base branch
gh pr create --base main --title "Add new feature" --body "Description"
```

### **Manage Pull Requests:**
```bash
gh pr view 1                   # View PR #1
gh pr close 1                  # Close PR #1 (don't merge)
gh pr reopen 1                 # Reopen PR #1
gh pr comment 1 --body "Looks good!" # Add comment to PR #1
```

### **Merge Pull Requests:**
```bash
# Normal merge (if no protection)
gh pr merge 1 --merge

# Merge with admin override (bypasses protection)
gh pr merge 1 --admin --merge --delete-branch

# Merge and delete branch
gh pr merge 1 --merge --delete-branch
```

## **🚨 Branch Protection Solutions:**

### **Option 1: Disable Protection (Easiest)**
1. **GitHub.com** → **Settings** → **Branches**
2. **Edit** your branch protection rule
3. **Uncheck "Require pull request reviews before merging"**
4. **Save changes**
5. **Merge your PR normally**
6. **Re-enable protection** if you want

### **Option 2: Use Admin Override**
```bash
# Find your PR number
gh pr list

# Merge with admin override (bypasses protection)
gh pr merge [PR_NUMBER] --admin --merge --delete-branch
```

### **Option 3: Add Collaborator**
1. **Create a second GitHub account**
2. **Add it as a collaborator** to your repository
3. **Use that account to review your own PRs**

## **💡 Common Workflows:**

### **Report a Bug:**
```bash
gh issue create --title "Bug: Compilation error in main.cpp" --body "Getting error: undefined reference to 'function_name'"
```

### **Request a Feature:**
```bash
gh issue create --title "Feature: Add WiFi support" --body "Need to add WiFi connectivity to the Pi Zero"
```

### **Submit Code Changes:**
```bash
# Make your changes
git add -A
git commit -m "Fix compilation errors"
git push origin feature/your-branch

# Create PR
gh pr create --title "Fix compilation errors" --body "Fixed all C++ compilation issues"
```

### **Merge Your Changes:**
```bash
# If no protection:
gh pr merge 1 --merge --delete-branch

# If protection blocks you:
gh pr merge 1 --admin --merge --delete-branch
```

## **🔧 Emergency Recovery:**

```bash
# Check your status
git status
git branch -a

# Get your branch back if needed
git checkout feature/your-branch-name

# List your PRs
gh pr list

# Merge with admin override
gh pr merge [PR_NUMBER] --admin --merge --delete-branch
```

## **📚 Quick Reference:**

| Action | Command |
|--------|---------|
| List issues | `gh issue list` |
| Create issue | `gh issue create --title "Title" --body "Description"` |
| Close issue | `gh issue close [NUMBER]` |
| List PRs | `gh pr list` |
| Create PR | `gh pr create --title "Title" --body "Description"` |
| Merge PR | `gh pr merge [NUMBER] --merge` |
| Merge with admin | `gh pr merge [NUMBER] --admin --merge --delete-branch` |
| View PR | `gh pr view [NUMBER]` |
| Close PR | `gh pr close [NUMBER]` |
