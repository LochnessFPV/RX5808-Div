# RX5808-Div Web Configurator Setup Guide

## Step 1: Fork ESP Web Tools Repository

### Option A: Via GitHub Web Interface (Easiest)
1. Go to: https://github.com/esphome/esp-web-tools
2. Click the "Fork" button (top right)
3. Choose your account (LochnessFPV)
4. **Change repository name to**: `RX5808-Div-Configurator`
5. **Description**: "Web-based firmware configurator for RX5808-Div FPV frequency scanner"
6. **Uncheck** "Copy the main branch only" (keep all branches)
7. Click "Create fork"

### Option B: Via GitHub CLI
```bash
# Install GitHub CLI if not already installed
# https://cli.github.com/

# Fork the repository
gh repo fork esphome/esp-web-tools --clone --fork-name RX5808-Div-Configurator
cd RX5808-Div-Configurator
```

### Option C: Manual Clone + Push to New Repo
```bash
# Clone ESP Web Tools
git clone https://github.com/esphome/esp-web-tools.git RX5808-Div-Configurator
cd RX5808-Div-Configurator

# Remove original remote
git remote remove origin

# Create new repo on GitHub first (via web interface), then:
git remote add origin https://github.com/LochnessFPV/RX5808-Div-Configurator.git
git push -u origin main
```

## Step 2: Clone to Local Machine

```powershell
# Navigate to your GitHub folder
cd C:\Users\DanieleLongo\Documents\GitHub\

# Clone your fork
git clone https://github.com/LochnessFPV/RX5808-Div-Configurator.git
cd RX5808-Div-Configurator
```

## Step 3: Explore the Structure

```powershell
# List files to understand structure
dir

# Open in VS Code
code .
```

### Key Files to Understand:
- `index.html` - Main page (this is what users see)
- `install-dialog.ts` - Flash dialog component
- `manifest.ts` - Manifest parsing logic
- `const.ts` - Constants and configuration
- `util/` - Utility functions
- `README.md` - Original documentation

## Step 4: Install Dependencies

ESP Web Tools uses npm for development:

```powershell
# Check if Node.js is installed
node --version
npm --version

# If not installed, download from: https://nodejs.org/

# Install dependencies
npm install

# Start development server
npm run dev
```

This will start a local server at `http://localhost:5173` (or similar)

## Step 5: Understand the Manifest Format

ESP Web Tools expects this structure (which we already generate!):

```json
{
  "name": "RX5808-Div",
  "version": "v1.7.0",
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        {
          "path": "rx5808-bootloader-v1.7.0.bin",
          "offset": 4096
        },
        {
          "path": "rx5808-partition-table-v1.7.0.bin",
          "offset": 32768
        },
        {
          "path": "rx5808-esp32-v1.7.0.bin",
          "offset": 65536
        }
      ]
    }
  ]
}
```

Perfect! This matches our generated manifest.json format.

## Step 6: First Customizations

### A. Update Project Metadata

Edit `package.json`:
```json
{
  "name": "rx5808-div-configurator",
  "description": "Web-based firmware configurator for RX5808-Div",
  "repository": "github:LochnessFPV/RX5808-Div-Configurator",
  "homepage": "https://lochnessfpv.github.io/RX5808-Div-Configurator/"
}
```

### B. Point to RX5808-Div Firmware

The key change is where it looks for firmware. You'll need to:

1. **Find where manifests are loaded** (typically `manifest.ts` or similar)
2. **Change the base URL** to:
   ```typescript
   const MANIFEST_BASE = "https://github.com/LochnessFPV/RX5808-Div/releases/download/";
   ```

### C. Update Branding

Edit `index.html` to replace "ESP Web Tools" branding with "RX5808-Div Configurator"

## Step 7: Test Locally

```powershell
# Run dev server
npm run dev

# Open browser to: http://localhost:5173
# Click "Connect" - it should show Web Serial dialog
# Test with your RX5808 hardware if available
```

## Step 8: Build for Production

```powershell
# Build optimized static files
npm run build

# Output will be in dist/ folder
# This is what you'll deploy to GitHub Pages
```

## Step 9: Deploy to GitHub Pages

### Enable GitHub Pages:
1. Go to your fork's Settings
2. Navigate to "Pages" (left sidebar)
3. Source: "GitHub Actions"
4. The repo should already have a deployment workflow

Or manually:
```powershell
# Build production files
npm run build

# Deploy to gh-pages branch
npm run deploy
```

Your configurator will be live at:
`https://lochnessfpv.github.io/RX5808-Div-Configurator/`

## 🎯 Immediate Next Steps

1. **Fork the repo** (Option A is easiest)
2. **Clone to local machine**
3. **Run `npm install` and `npm run dev`**
4. **Explore the codebase** - understand how it works
5. **Test with your hardware** - connect your RX5808-Div

## 📚 Resources

- ESP Web Tools: https://esphome.github.io/esp-web-tools/
- Web Serial API: https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API
- GitHub Pages: https://pages.github.com/

## ⚠️ Prerequisites

- **Node.js** (v16 or newer): https://nodejs.org/
- **Chrome/Edge browser** (Web Serial API support)
- **Windows with USB drivers** for ESP32

## 🔄 Alternative: Start from Scratch

If ESP Web Tools is too complex, I can help you build a simpler version from scratch using:
- Plain HTML/JS (no build step)
- esptool-js library directly
- Fetch releases from GitHub API
- Custom UI tailored for RX5808-Div

Let me know which approach you prefer!

---

**Next Steps After Setup:**
1. Understand the codebase structure
2. Modify manifest URL to point to RX5808-Div releases
3. Customize branding and UI
4. Add RX5808-Div specific features
5. Test with real hardware
6. Deploy to GitHub Pages
