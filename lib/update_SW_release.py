import os
import re
import json
import shutil
import subprocess
import sys

# ==============================
# CONFIG
# ==============================
SOURCE_PROJECT = r"C:\Users\LEONDaniel\Documents\GitHub\Debbie_Drip_Irrigation_System"
RELEASE_REPO = r"C:\Users\LEONDaniel\Documents\GitHub\Releases\Irrigation_System"

ENV_NAME = "esp32s3"

INCLUDE_FILE = os.path.join(SOURCE_PROJECT, "src", "helper.cpp")
FIRMWARE_SRC = os.path.join(SOURCE_PROJECT, ".pio", "build", ENV_NAME, "firmware.bin")

MANIFEST_FILE = os.path.join(RELEASE_REPO, "manifest.json")
FIRMWARE_DEST = os.path.join(RELEASE_REPO, "irrigation.bin")


# ==============================
# PlatformIO runner (VS Code install safe)
# ==============================
def run_platformio_build():
    print("\n🔧 Building firmware (live output)...\n")

    pio_exe = os.path.expanduser(r"~\.platformio\penv\Scripts\platformio.exe")

    if not os.path.exists(pio_exe):
        raise RuntimeError(
            f"PlatformIO not found at:\n{pio_exe}\n"
            "Install PlatformIO VS Code extension first."
        )

    process = subprocess.Popen(
        [pio_exe, "run", "-e", ENV_NAME],
        cwd=SOURCE_PROJECT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    # Stream output live
    for line in process.stdout:
        print(line, end="")

    process.wait()

    if process.returncode != 0:
        raise RuntimeError("❌ PlatformIO build failed")

    print("\n✅ Build complete\n")


# ==============================
# Extract version
# ==============================
def extract_version():
    print("\n🔍 Extracting version...")

    with open(INCLUDE_FILE, "r") as f:
        content = f.read()

    match = re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]+)"', content)

    if not match:
        raise ValueError("Version not found in helper.cpp")

    version = match.group(1)
    print(f"✅ Version: {version}")
    return version


# ==============================
# Update manifest
# ==============================
def update_manifest(version):
    print("\n📝 Updating manifest...")

    with open(MANIFEST_FILE, "r") as f:
        data = json.load(f)

    data["version"] = version

    with open(MANIFEST_FILE, "w") as f:
        json.dump(data, f, indent=4)

    print("✅ manifest.json updated")


# ==============================
# Copy firmware
# ==============================
def copy_firmware():
    print("\n📦 Copying firmware...")

    if not os.path.exists(FIRMWARE_SRC):
        raise FileNotFoundError("firmware.bin missing (build failed?)")

    shutil.copy2(FIRMWARE_SRC, FIRMWARE_DEST)

    print(f"✅ Copied {FIRMWARE_SRC} → {FIRMWARE_DEST}")


# ==============================
# Git release commit
# ==============================
def git_release(version):
    print("\n📤 Committing release...")

    subprocess.run(
        ["git", "add", "manifest.json", "irrigation.bin"],
        cwd=RELEASE_REPO,
        check=True
    )

    status = subprocess.run(
        ["git", "status", "--porcelain"],
        cwd=RELEASE_REPO,
        capture_output=True,
        text=True
    )

    if not status.stdout.strip():
        print("ℹ️ No changes to commit")
        return

    subprocess.run(
        ["git", "commit", "-m", f"Release {version}"],
        cwd=RELEASE_REPO,
        check=True
    )

    # tag
    tag_check = subprocess.run(
        ["git", "tag", "-l", version],
        cwd=RELEASE_REPO,
        capture_output=True,
        text=True
    )

    if version not in tag_check.stdout:
        subprocess.run(["git", "tag", version], cwd=RELEASE_REPO, check=True)
        print(f"🏷️ Tagged {version}")

    subprocess.run(["git", "push"], cwd=RELEASE_REPO, check=True)
    subprocess.run(["git", "push", "origin", version], cwd=RELEASE_REPO, check=True)

    print("🚀 Pushed release")


# ==============================
# MAIN ONE-CLICK FLOW
# ==============================
def main():
    print("\n==============================")
    print("🚀 ONE CLICK RELEASE START")
    print("==============================")

    run_platformio_build()

    version = extract_version()
    update_manifest(version)
    copy_firmware()
    git_release(version)

    print("\n==============================")
    print("🎉 RELEASE COMPLETE")
    print("==============================\n")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("\n❌ ERROR:", e)
        sys.exit(1)