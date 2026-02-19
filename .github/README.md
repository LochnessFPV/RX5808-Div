# Release Automation

This directory contains GitHub Actions workflows and templates for automated firmware builds and releases.

## Automated Build Workflow

The `build-firmware.yml` workflow automatically:
- Builds ESP32 firmware when a new release is created
- Generates properly named firmware files with version tags
- Creates a `manifest.json` for web-based firmware updates
- Uploads all files to the GitHub release

### Manual Trigger

You can also trigger the build manually:
1. Go to Actions → Build ESP32 Firmware
2. Click "Run workflow"
3. Enter the version tag (e.g., `v1.7.0`)
4. The build artifacts will be available for download

## Release Process

### Creating a New Release

1. **Update version in code** (if applicable)
2. **Update CHANGELOG.md** with new version section
3. **Commit changes** to main branch
4. **Create and push tag**:
   ```bash
   git tag v1.7.0
   git push origin v1.7.0
   ```
5. **Create GitHub release**:
   - Go to Releases → Draft a new release
   - Choose the tag you just created
   - Fill in release title: "v1.7.0 - Description"
   - Add release notes (copy from CHANGELOG.md)
   - Check "Set as latest release"
   - Click "Publish release"
6. **Wait for automation**: The GitHub Actions workflow will automatically build and upload firmware files

### Release Assets Structure

Each release should contain:
- `rx5808-esp32-v1.x.x.bin` - Main application firmware
- `rx5808-bootloader-v1.x.x.bin` - ESP32 bootloader
- `rx5808-partition-table-v1.x.x.bin` - Partition table
- `manifest.json` - Web configurator metadata

### Manifest Format

The `manifest.json` follows the ESP Web Tools format:

```json
{
  "name": "RX5808-Div",
  "version": "v1.7.0",
  "build_date": "2026-02-19T00:00:00Z",
  "commit": "69ce73f",
  "changelog_url": "https://github.com/Ft-Available/RX5808-Div/blob/main/CHANGELOG.md#v170---2026-02-19",
  "documentation_url": "https://github.com/Ft-Available/RX5808-Div#readme",
  "new_install_improv_wait_time": 0,
  "new_install_prompt_erase": true,
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

## Updating Existing Releases

For releases created before automation:
1. Build firmware manually: `cd Firmware/ESP32/RX5808 && idf.py build`
2. Copy `manifest-template.json` and update version/commit
3. Rename firmware files with version suffix
4. Upload to existing release using GitHub web interface or `gh` CLI

## Web Configurator Integration

The web configurator (separate repository) will:
- Fetch the list of releases from GitHub API
- Parse `manifest.json` from each release
- Present versions to users for flashing
- Use Web Serial API + esptool-js to flash firmware

### Manifest URLs

The web configurator accesses manifests at:
```
https://github.com/Ft-Available/RX5808-Div/releases/download/v1.7.0/manifest.json
```

Firmware files are downloaded from:
```
https://github.com/Ft-Available/RX5808-Div/releases/download/v1.7.0/rx5808-esp32-v1.7.0.bin
```
