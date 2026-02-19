#!/usr/bin/env python3
"""
Generate manifest.json for RX5808-Div firmware releases.

Usage:
    python generate_manifest.py --version v1.7.0 --commit 69ce73f --build-dir Firmware/ESP32/RX5808/build

This script creates a manifest.json file compatible with ESP Web Tools for web-based firmware flashing.
"""

import argparse
import json
import os
from datetime import datetime


def get_file_size(filepath):
    """Get file size in bytes, return 0 if file doesn't exist."""
    try:
        return os.path.getsize(filepath)
    except FileNotFoundError:
        return 0


def generate_manifest(version, commit, build_dir, output_file):
    """Generate manifest.json for web configurator."""
    
    # Base URLs for GitHub releases
    repo_url = "https://github.com/Ft-Available/RX5808-Div"
    release_base = f"{repo_url}/releases/download/{version}"
    
    # Firmware file names
    app_bin = f"rx5808-esp32-{version}.bin"
    bootloader_bin = f"rx5808-bootloader-{version}.bin"
    partition_bin = f"rx5808-partition-table-{version}.bin"
    
    # Check if files exist (for validation)
    app_path = os.path.join(build_dir, app_bin)
    bootloader_path = os.path.join(build_dir, bootloader_bin)
    partition_path = os.path.join(build_dir, partition_bin)
    
    app_size = get_file_size(app_path)
    bootloader_size = get_file_size(bootloader_path)
    partition_size = get_file_size(partition_path)
    
    # Build timestamp
    build_date = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    
    # Version for changelog URL (remove 'v' prefix and format for anchor)
    changelog_version = version.lstrip('v').replace('.', '')
    
    manifest = {
        "name": "RX5808-Div",
        "version": version,
        "build_date": build_date,
        "commit": commit,
        "changelog_url": f"{repo_url}/blob/main/CHANGELOG.md#v{changelog_version}---{datetime.utcnow().strftime('%Y-%m-%d')}",
        "documentation_url": f"{repo_url}#readme",
        "new_install_improv_wait_time": 0,
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": [
                    {
                        "path": bootloader_bin,
                        "offset": 4096
                    },
                    {
                        "path": partition_bin,
                        "offset": 32768
                    },
                    {
                        "path": app_bin,
                        "offset": 65536
                    }
                ]
            }
        ]
    }
    
    # Write manifest
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)
    
    print(f"✓ Generated manifest: {output_file}")
    print(f"  Version: {version}")
    print(f"  Commit: {commit}")
    print(f"  Build date: {build_date}")
    
    # Validation warnings
    if app_size == 0:
        print(f"  ⚠ Warning: Application binary not found: {app_path}")
    else:
        print(f"  Application: {app_bin} ({app_size:,} bytes)")
    
    if bootloader_size == 0:
        print(f"  ⚠ Warning: Bootloader binary not found: {bootloader_path}")
    else:
        print(f"  Bootloader: {bootloader_bin} ({bootloader_size:,} bytes)")
    
    if partition_size == 0:
        print(f"  ⚠ Warning: Partition table not found: {partition_path}")
    else:
        print(f"  Partition table: {partition_bin} ({partition_size:,} bytes)")
    
    print(f"\nNext steps:")
    print(f"1. Rename firmware files in build directory:")
    print(f"   cp RX5808.bin {app_bin}")
    print(f"   cp bootloader/bootloader.bin {bootloader_bin}")
    print(f"   cp partition_table/partition-table.bin {partition_bin}")
    print(f"2. Upload to GitHub release:")
    print(f"   gh release upload {version} {app_bin} {bootloader_bin} {partition_bin} manifest.json")


def main():
    parser = argparse.ArgumentParser(
        description='Generate manifest.json for RX5808-Div firmware release'
    )
    parser.add_argument(
        '--version',
        required=True,
        help='Release version (e.g., v1.7.0)'
    )
    parser.add_argument(
        '--commit',
        required=True,
        help='Git commit hash (short or full)'
    )
    parser.add_argument(
        '--build-dir',
        default='Firmware/ESP32/RX5808/build',
        help='Build directory containing firmware files'
    )
    parser.add_argument(
        '--output',
        default='manifest.json',
        help='Output file path (default: manifest.json)'
    )
    
    args = parser.parse_args()
    
    # Ensure version starts with 'v'
    version = args.version if args.version.startswith('v') else f"v{args.version}"
    
    # Create output directory if needed
    output_dir = os.path.dirname(args.output) or '.'
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate manifest
    generate_manifest(
        version=version,
        commit=args.commit,
        build_dir=args.build_dir,
        output_file=args.output
    )


if __name__ == '__main__':
    main()
