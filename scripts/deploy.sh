#!/bin/bash
# Deploy Geomys to Snow disk image
# Kills Snow if running, updates the disk image, optionally relaunches
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
DISK_IMG="$SCRIPT_DIR/diskimages/snow-sys608.img"
SNOW_CFG="$SCRIPT_DIR/diskimages/geomys.snoww"
SNOW_BIN="$SCRIPT_DIR/tools/snow/snowemu"

# Kill Snow if running, wait for it to exit
kill_snow() {
    if pgrep -f snowemu > /dev/null 2>&1; then
        echo "Stopping Snow..."
        pkill -f snowemu || true
        # Wait up to 5 seconds for it to exit
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

# Deploy to HFS image
deploy() {
    if [ ! -f "$BUILD_DIR/Geomys.bin" ]; then
        echo "Error: No build found. Run ./scripts/build.sh first."
        exit 1
    fi

    echo "Deploying to $DISK_IMG..."
    hmount "$DISK_IMG"
    hcopy -m "$BUILD_DIR/Geomys.bin" ':Geomys:Geomys'
    hattrib -t APPL -c GEOM ':Geomys:Geomys'
    if [ -f "$BUILD_DIR/About Geomys" ]; then
        hcopy -r "$BUILD_DIR/About Geomys" ':Geomys:About Geomys'
        hattrib -t ttro -c ttxt ':Geomys:About Geomys'
    fi
    humount
    echo "Deployed."
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
kill_snow
deploy

if [ "$1" = "--no-launch" ]; then
    echo "Skipping Snow launch (--no-launch)."
else
    launch_snow
fi
