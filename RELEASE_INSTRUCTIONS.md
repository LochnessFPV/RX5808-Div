# Release v1.7.0 - Setup Instructions

## ✅ What Was Done

### 1. Release Automation Infrastructure
- ✅ Created GitHub Actions workflow (`.github/workflows/build-firmware.yml`)
- ✅ Workflow triggers on release creation or manual dispatch
- ✅ Automatically builds firmware and generates manifest.json
- ✅ Uploads all assets to GitHub releases

### 2. Release Tools
- ✅ Python script to generate manifest.json (`generate_manifest.py`)
- ✅ Manifest template for manual use
- ✅ Comprehensive documentation in `.github/README.md`

### 3. v1.7.0 Release Assets (Ready!)
- ✅ `rx5808-esp32-v1.7.0.bin` (1,058,976 bytes)
- ✅ `rx5808-bootloader-v1.7.0.bin` (26,272 bytes)
- ✅ `rx5808-partition-table-v1.7.0.bin` (3,072 bytes)
- ✅ `manifest.json` (web configurator metadata)

**Location**: `Firmware/ESP32/RX5808/build/`

### 4. Git Repository
- ✅ Commit `bbcc183` - CI/CD infrastructure
- ✅ Tag `v1.7.0` created
- ✅ Pushed to origin/main

## 📋 Next Steps: Create GitHub Release

### Step 1: Go to GitHub Releases
1. Visit: https://github.com/LochnessFPV/RX5808-Div/releases
2. Click "Draft a new release"

### Step 2: Configure Release
- **Tag**: Select `v1.7.0` (already created)
- **Title**: `v1.7.0 - ELRS Wireless VRX Backpack Support`
- **Description**: Copy content from `RELEASE_NOTES_v1.7.0.md`
- **Attach binaries**: Upload these 4 files from `Firmware/ESP32/RX5808/build/`:
  1. `rx5808-esp32-v1.7.0.bin`
  2. `rx5808-bootloader-v1.7.0.bin`
  3. `rx5808-partition-table-v1.7.0.bin`
  4. `manifest.json`

### Step 3: Publish
- ✅ Check "Set as the latest release"
- Click "Publish release"

## 🔄 Future Releases (Fully Automated!)

For all future releases, the process is simplified:

### Simple Process:
1. Update CHANGELOG.md
2. Commit changes
3. Create and push tag:
   ```bash
   git tag v1.8.0 -m "v1.8.0 - Description"
   git push origin v1.8.0
   ```
4. Create GitHub release (just add description)
5. **GitHub Actions builds everything automatically!**

The workflow will:
- ✅ Build ESP32 firmware from source
- ✅ Generate all .bin files with version suffix
- ✅ Create manifest.json
- ✅ Upload everything to the release

## 🌐 Web Configurator (Next Phase)

Once this release is published, you can start building the web configurator:

### Repository Structure:
```
RX5808-Div-Configurator/
  ├── index.html
  ├── app.js
  ├── manifest-loader.js
  ├── esptool-js/
  └── README.md
```

### Configurator Will:
1. Fetch releases from GitHub API
2. Parse manifest.json for each version
3. Display available versions to user
4. Flash selected firmware via Web Serial API
5. Use esptool-js for ESP32 programming

### Technology Stack:
- Plain HTML/CSS/JavaScript (no build step)
- Web Serial API (Chrome 89+)
- esptool-js library
- GitHub Pages for hosting

### Timeline:
- Release v1.7.0: **NOW** (manual asset upload)
- Web configurator MVP: **2-3 days**
- Future releases: **Fully automated**

## 📁 File Locations

### Release Assets (ready to upload):
```
C:\Users\DanieleLongo\Documents\GitHub\RX5808-Div\Firmware\ESP32\RX5808\build\
  ├── rx5808-esp32-v1.7.0.bin
  ├── rx5808-bootloader-v1.7.0.bin
  ├── rx5808-partition-table-v1.7.0.bin
  └── manifest.json
```

### Release Notes:
```
C:\Users\DanieleLongo\Documents\GitHub\RX5808-Div\RELEASE_NOTES_v1.7.0.md
```

### CI/CD Infrastructure:
```
C:\Users\DanieleLongo\Documents\GitHub\RX5808-Div\.github\
  ├── workflows/build-firmware.yml
  ├── generate_manifest.py
  ├── manifest-template.json
  └── README.md
```

## 🎯 Summary

**Current Status**: Repository restructured and future-proofed! ✅
- CI/CD automation ready
- v1.7.0 assets prepared
- Manifest.json generated
- Git tag pushed

**Immediate Action**: Create GitHub release and upload the 4 files

**After Release**: Build web configurator in separate repo

---

**Questions?** All infrastructure is documented in `.github/README.md`
