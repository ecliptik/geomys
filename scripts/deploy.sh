#!/bin/bash
# Deploy Geomys to Snow disk image or Basilisk II shared directory
# Usage: deploy.sh [--target snow|basilisk] [--no-launch]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
EMU_DIR="/home/claude/emulators"
DISK_IMG="$EMU_DIR/disks/system6.img"
SNOW_CFG="$EMU_DIR/snow/geomys.snoww"
SNOW_BIN="$EMU_DIR/snow/snowemu"
BASILISK_UNIX="$EMU_DIR/unix"

TARGET="snow"
NO_LAUNCH=false
AUTO_OPEN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --target)    TARGET="$2"; shift 2 ;;
        --no-launch) NO_LAUNCH=true; shift ;;
        --auto-open) AUTO_OPEN=true; shift ;;
        *)           shift ;;
    esac
done

# Kill Snow if running, wait for it to exit
kill_snow() {
    if pgrep -f snowemu > /dev/null 2>&1; then
        echo "Stopping Snow..."
        pkill -f snowemu || true
        for i in $(seq 1 10); do
            if ! pgrep -f snowemu > /dev/null 2>&1; then
                echo "Snow stopped."
                return 0
            fi
            sleep 0.5
        done
        echo "Snow didn't stop gracefully, sending SIGKILL..."
        pkill -9 -f snowemu || true
        sleep 0.5
    fi
}

# Deploy to Snow HFS disk image
deploy_snow() {
    if [ ! -f "$BUILD_DIR/Geomys.bin" ]; then
        echo "Error: No build found. Run ./scripts/build.sh first."
        exit 1
    fi

    kill_snow

    echo "Deploying to $DISK_IMG..."
    hmount "$DISK_IMG"
    hcopy -m "$BUILD_DIR/Geomys.bin" ':Geomys:Geomys'
    hattrib -t APPL -c GEOM ':Geomys:Geomys'
    if [ -f "$BUILD_DIR/About Geomys" ]; then
        hcopy -r "$BUILD_DIR/About Geomys" ':Geomys:About Geomys'
        hattrib -t ttro -c ttxt ':Geomys:About Geomys'
    fi
    humount
    echo "Deployed to Snow disk image."

    if [ "$NO_LAUNCH" = false ]; then
        launch_snow
        if [ "$AUTO_OPEN" = true ]; then
            echo "Auto-opening Geomys in 12s..."
            python3 /home/claude/emulators/scripts/open_geomys.py 12 &
        fi
    fi
}

# Deploy to Basilisk II shared directory (extfs)
deploy_basilisk() {
    if [ ! -f "$BUILD_DIR/Geomys.bin" ]; then
        echo "Error: No build found. Run ./scripts/build.sh first."
        exit 1
    fi

    mkdir -p "$BASILISK_UNIX"
    # Extract MacBinary into extfs format (data fork + resource fork + Finder info)
    python3 "$EMU_DIR/basilisk/macbin_to_extfs.py" "$BUILD_DIR/Geomys.bin" "$BASILISK_UNIX" Geomys
    if [ -f "$BUILD_DIR/About Geomys" ]; then
        cp "$BUILD_DIR/About Geomys" "$BASILISK_UNIX/About Geomys"
    fi
    echo "Deployed to $BASILISK_UNIX/"
    echo "In Basilisk II: open Geomys from the Unix volume."
}

# Launch Snow
launch_snow() {
    if [ -z "$DISPLAY" ]; then
        export DISPLAY=:0
    fi
    echo "Launching Snow..."
    "$SNOW_BIN" "$SNOW_CFG" &
    echo "Snow running (PID $!)."
}

# Main
case "$TARGET" in
    snow)     deploy_snow ;;
    basilisk) deploy_basilisk ;;
    *)        echo "Unknown target: $TARGET (use snow or basilisk)"; exit 1 ;;
esac
