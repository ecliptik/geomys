#!/bin/bash
# Build Geomys for classic Macintosh using Retro68 toolchain
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TOOLCHAIN="$SCRIPT_DIR/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake"
BUILD_DIR="$SCRIPT_DIR/build"

# --- Feature flag defaults (= full preset, all ON) ---
GEOMYS_OFFSCREEN=ON
GEOMYS_STATUS_BAR=ON
GEOMYS_FAVORITES=ON
GEOMYS_COLOR=OFF
GEOMYS_DOWNLOAD=ON
GEOMYS_GOPHER_PLUS=ON
GEOMYS_GLYPHS=ON
GEOMYS_CP437=ON
GEOMYS_STYLES=ON
GEOMYS_CACHE=ON
GEOMYS_CLIPBOARD=ON
GEOMYS_MAX_WINDOWS=1

PRESET=""

# --- Apply preset ---
apply_preset() {
    case "$1" in
        minimal)
            GEOMYS_OFFSCREEN=OFF
            GEOMYS_STATUS_BAR=ON
            GEOMYS_FAVORITES=OFF
            GEOMYS_COLOR=OFF
            GEOMYS_DOWNLOAD=OFF
            GEOMYS_GOPHER_PLUS=OFF
            GEOMYS_GLYPHS=OFF
            GEOMYS_CP437=OFF
            GEOMYS_STYLES=OFF
            GEOMYS_CACHE=OFF
            GEOMYS_CLIPBOARD=OFF
            GEOMYS_MAX_WINDOWS=1
            ;;
        lite|macplus)
            GEOMYS_OFFSCREEN=ON
            GEOMYS_STATUS_BAR=ON
            GEOMYS_FAVORITES=ON
            GEOMYS_COLOR=OFF
            GEOMYS_DOWNLOAD=OFF
            GEOMYS_GOPHER_PLUS=OFF
            GEOMYS_GLYPHS=OFF
            GEOMYS_CP437=ON
            GEOMYS_STYLES=OFF
            GEOMYS_CACHE=OFF
            GEOMYS_CLIPBOARD=ON
            GEOMYS_MAX_WINDOWS=2
            ;;
        full|default)
            GEOMYS_OFFSCREEN=ON
            GEOMYS_STATUS_BAR=ON
            GEOMYS_FAVORITES=ON
            GEOMYS_COLOR=OFF
            GEOMYS_DOWNLOAD=ON
            GEOMYS_GOPHER_PLUS=ON
            GEOMYS_GLYPHS=ON
            GEOMYS_CP437=ON
            GEOMYS_STYLES=ON
            GEOMYS_CACHE=ON
            GEOMYS_CLIPBOARD=ON
            GEOMYS_MAX_WINDOWS=4
            ;;
        *)
            echo "Error: unknown preset '$1' (valid: minimal, lite, full; macplus is alias for lite)"
            exit 1
            ;;
    esac
}

# --- Parse command-line flags ---
MAKE_ARGS=()
ARGS=("$@")
i=0
# First pass: find and apply preset
while [ $i -lt ${#ARGS[@]} ]; do
    case "${ARGS[$i]}" in
        --preset)
            PRESET="${ARGS[$((i+1))]}"
            apply_preset "$PRESET"
            i=$((i + 2))
            ;;
        --max-windows)
            i=$((i + 2))
            ;;
        *)
            i=$((i + 1))
            ;;
    esac
done

# Second pass: apply individual overrides
while [[ $# -gt 0 ]]; do
    case $1 in
        --preset)        shift 2 ;;  # already handled
        --offscreen)     GEOMYS_OFFSCREEN=ON;      shift ;;
        --no-offscreen)  GEOMYS_OFFSCREEN=OFF;     shift ;;
        --statusbar)     GEOMYS_STATUS_BAR=ON;     shift ;;
        --no-statusbar)  GEOMYS_STATUS_BAR=OFF;    shift ;;
        --favorites)     GEOMYS_FAVORITES=ON;      shift ;;
        --no-favorites)  GEOMYS_FAVORITES=OFF;     shift ;;
        --color)         GEOMYS_COLOR=ON;           shift ;;
        --no-color)      GEOMYS_COLOR=OFF;          shift ;;
        --download)      GEOMYS_DOWNLOAD=ON;        shift ;;
        --no-download)   GEOMYS_DOWNLOAD=OFF;       shift ;;
        --gopher-plus)   GEOMYS_GOPHER_PLUS=ON;    shift ;;
        --no-gopher-plus) GEOMYS_GOPHER_PLUS=OFF;  shift ;;
        --glyphs)        GEOMYS_GLYPHS=ON;          shift ;;
        --no-glyphs)     GEOMYS_GLYPHS=OFF;         shift ;;
        --cp437)         GEOMYS_CP437=ON;            shift ;;
        --no-cp437)      GEOMYS_CP437=OFF;           shift ;;
        --styles)        GEOMYS_STYLES=ON;           shift ;;
        --no-styles)     GEOMYS_STYLES=OFF;          shift ;;
        --cache)         GEOMYS_CACHE=ON;            shift ;;
        --no-cache)      GEOMYS_CACHE=OFF;           shift ;;
        --clipboard)     GEOMYS_CLIPBOARD=ON;        shift ;;
        --no-clipboard)  GEOMYS_CLIPBOARD=OFF;       shift ;;
        --max-windows)   GEOMYS_MAX_WINDOWS="$2";    shift 2 ;;
        *)
            MAKE_ARGS+=("$1")
            shift
            ;;
    esac
done

# --- Dependency resolution ---
# CP437 requires GLYPHS for full rendering
if [ "$GEOMYS_CP437" = "ON" ] && [ "$GEOMYS_GLYPHS" = "OFF" ]; then
    echo "Note: --cp437 requires --glyphs for full rendering, enabling it"
    GEOMYS_GLYPHS=ON
fi

# --- Compute SIZE resource partition ---
compute_size() {
    local base=100

    # Shared (global) memory costs
    local shared=0
    [ "$GEOMYS_OFFSCREEN" = "ON" ] && shared=$(( shared + 22 ))
    [ "$GEOMYS_STATUS_BAR" = "ON" ] && shared=$(( shared + 1 ))
    [ "$GEOMYS_FAVORITES" = "ON" ] && shared=$(( shared + 3 ))
    [ "$GEOMYS_GLYPHS" = "ON" ] && shared=$(( shared + 6 ))
    [ "$GEOMYS_CP437" = "ON" ] && shared=$(( shared + 1 ))
    [ "$GEOMYS_GOPHER_PLUS" = "ON" ] && shared=$(( shared + 3 ))

    # Per-session memory: items(80KB) + text(32KB) + tcp(12KB) + history(4KB) = 128KB
    local per_session=128
    [ "$GEOMYS_CACHE" = "ON" ] && per_session=$(( per_session + 100 ))
    local session_total=$(( per_session * GEOMYS_MAX_WINDOWS ))

    # Total with 30% headroom
    local computed=$(( base + shared + session_total ))
    SIZE_PREFERRED=$(( computed * 130 / 100 ))
    SIZE_MINIMUM=$(( SIZE_PREFERRED - 128 ))

    # Clamp — keep SIZE modest to avoid starving system heap
    # on 4MB Mac Plus. Original single-window: 384/256.
    [ $SIZE_PREFERRED -lt 256 ] && SIZE_PREFERRED=256 || true
    [ $SIZE_MINIMUM -lt 192 ] && SIZE_MINIMUM=192 || true
    [ $SIZE_PREFERRED -gt 384 ] && SIZE_PREFERRED=384 || true
    [ $SIZE_MINIMUM -gt 256 ] && SIZE_MINIMUM=256 || true
}

compute_size

# --- Toolchain check ---
if [ ! -f "$TOOLCHAIN" ]; then
    echo "Error: Retro68 toolchain not found at $TOOLCHAIN"
    echo "Build it first:"
    echo "  cd Retro68-build && bash ../Retro68/build-toolchain.bash --no-ppc --no-carbon --prefix=\$(pwd)/toolchain"
    exit 1
fi

# Read version from CMakeLists.txt
VERSION=$(grep -oP 'project\(Geomys VERSION \K[0-9]+\.[0-9]+\.[0-9]+' "$SCRIPT_DIR/CMakeLists.txt")
if [ -z "$VERSION" ]; then
    echo "Warning: Could not read version from CMakeLists.txt, using 'unknown'"
    VERSION="unknown"
fi

# Compute display version: tagged release → version, dev build → short SHA
SHORT_SHA=$(git -C "$SCRIPT_DIR" rev-parse --short HEAD 2>/dev/null || echo "")
GIT_TAG=$(git -C "$SCRIPT_DIR" tag --points-at HEAD 2>/dev/null | grep -x "v${VERSION}" || true)
if [ -n "$GIT_TAG" ]; then
    VERSION_DISPLAY="${VERSION}"
elif [ -n "$SHORT_SHA" ]; then
    VERSION_DISPLAY="${SHORT_SHA}"
else
    VERSION_DISPLAY="${VERSION}"
fi

# Stamp version into resource file for About dialog before building
REZ_FILE="$SCRIPT_DIR/resources/geomys.r"
REZ_BACKUP="$BUILD_DIR/.geomys.r.bak"
mkdir -p "$BUILD_DIR"
cp "$REZ_FILE" "$REZ_BACKUP"
sed -i "s/\"Geomys ${VERSION}\"/\"Geomys ${VERSION_DISPLAY}\"/" "$REZ_FILE"

# Stamp SIZE resource partition
sed -i "s/384 \* 1024/${SIZE_PREFERRED} * 1024/" "$REZ_FILE"
sed -i "s/256 \* 1024/${SIZE_MINIMUM} * 1024/" "$REZ_FILE"

# Build (restore .r file on exit, even if build fails)
cleanup() { cp "$REZ_BACKUP" "$REZ_FILE" 2>/dev/null; rm -f "$REZ_BACKUP"; }
trap cleanup EXIT

cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DGEOMYS_OFFSCREEN="$GEOMYS_OFFSCREEN" \
    -DGEOMYS_STATUS_BAR="$GEOMYS_STATUS_BAR" \
    -DGEOMYS_FAVORITES="$GEOMYS_FAVORITES" \
    -DGEOMYS_COLOR="$GEOMYS_COLOR" \
    -DGEOMYS_DOWNLOAD="$GEOMYS_DOWNLOAD" \
    -DGEOMYS_GOPHER_PLUS="$GEOMYS_GOPHER_PLUS" \
    -DGEOMYS_GLYPHS="$GEOMYS_GLYPHS" \
    -DGEOMYS_CP437="$GEOMYS_CP437" \
    -DGEOMYS_STYLES="$GEOMYS_STYLES" \
    -DGEOMYS_CACHE="$GEOMYS_CACHE" \
    -DGEOMYS_CLIPBOARD="$GEOMYS_CLIPBOARD" \
    -DGEOMYS_MAX_WINDOWS="$GEOMYS_MAX_WINDOWS"
make "${MAKE_ARGS[@]}"

# Fix creator code in MacBinary header (Retro68 sets '????' instead of 'GEOM')
# Then recalculate MacBinary II CRC-16 (XMODEM) over header bytes 0-123
printf 'GEOM' | dd of="$BUILD_DIR/Geomys.bin" bs=1 seek=69 count=4 conv=notrunc 2>/dev/null
python3 -c "
import struct
with open('$BUILD_DIR/Geomys.bin', 'r+b') as f:
    hdr = bytearray(f.read(128))
    crc = 0
    for b in hdr[:124]:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021 if crc & 0x8000 else crc << 1) & 0xFFFF
    f.seek(124)
    f.write(struct.pack('>H', crc))
"

# Generate BinHex (.hqx) archive if macutils is available
if command -v binhex >/dev/null 2>&1; then
    binhex "$BUILD_DIR/Geomys.bin" > "$BUILD_DIR/Geomys.hqx"
    echo "BinHex archive created: Geomys.hqx"
else
    echo "Note: Install macutils for BinHex output: sudo apt install macutils"
fi

# Convert About Geomys line endings to Mac CR format, stamping display version
ABOUT_SRC="$SCRIPT_DIR/docs/About Geomys"
ABOUT_OUT="$BUILD_DIR/About Geomys"
if [ -f "$ABOUT_SRC" ]; then
    sed "s/Version ${VERSION}/Version ${VERSION_DISPLAY}/" "$ABOUT_SRC" | tr '\n' '\r' > "$ABOUT_OUT"
fi

# Post-process 800K floppy image: set creator code and add About Geomys
if [ -f "$BUILD_DIR/Geomys.dsk" ]; then
    hmount "$BUILD_DIR/Geomys.dsk"
    hattrib -t APPL -c GEOM :Geomys
    if [ -f "$ABOUT_OUT" ]; then
        hcopy -r "$ABOUT_OUT" ":About Geomys"
        hattrib -t ttro -c ttxt ":About Geomys"
    fi
    humount
fi

# --- Determine file prefix from preset ---
PRESET_LABEL="${PRESET:-full}"
[ "$PRESET_LABEL" = "macplus" ] && PRESET_LABEL="lite"
[ "$PRESET_LABEL" = "default" ] && PRESET_LABEL="full"
case "$PRESET_LABEL" in
    full)    FILE_PREFIX="Geomys" ;;
    minimal) FILE_PREFIX="Geomys-Minimal" ;;
    *)       FILE_PREFIX="Geomys-Lite" ;;
esac

# Create versioned copies with preset in filename
cp "$BUILD_DIR/Geomys.bin" "$BUILD_DIR/${FILE_PREFIX}-${VERSION_DISPLAY}.bin"
cp "$BUILD_DIR/Geomys.dsk" "$BUILD_DIR/${FILE_PREFIX}-${VERSION_DISPLAY}.dsk"
[ -f "$BUILD_DIR/Geomys.hqx" ] && cp "$BUILD_DIR/Geomys.hqx" "$BUILD_DIR/${FILE_PREFIX}-${VERSION_DISPLAY}.hqx"

# --- Build summary ---
ENABLED=""
DISABLED=""
for feat in offscreen statusbar favorites gopher-plus glyphs cp437 styles cache clipboard color download; do
    case $feat in
        offscreen)    val=$GEOMYS_OFFSCREEN ;;
        statusbar)    val=$GEOMYS_STATUS_BAR ;;
        favorites)    val=$GEOMYS_FAVORITES ;;
        gopher-plus)  val=$GEOMYS_GOPHER_PLUS ;;
        glyphs)       val=$GEOMYS_GLYPHS ;;
        cp437)        val=$GEOMYS_CP437 ;;
        styles)       val=$GEOMYS_STYLES ;;
        cache)        val=$GEOMYS_CACHE ;;
        clipboard)    val=$GEOMYS_CLIPBOARD ;;
        color)        val=$GEOMYS_COLOR ;;
        download)     val=$GEOMYS_DOWNLOAD ;;
    esac
    if [ "$val" = "ON" ]; then
        ENABLED="$ENABLED $feat"
    else
        DISABLED="$DISABLED $feat"
    fi
done

echo ""
echo "Build complete (v${VERSION_DISPLAY}, ${PRESET_LABEL} preset):"
echo "  Features:${ENABLED}"
[ -n "$DISABLED" ] && echo "  Disabled:${DISABLED}"
echo "  Windows: ${GEOMYS_MAX_WINDOWS} max"
echo "  SIZE: ${SIZE_PREFERRED}KB preferred / ${SIZE_MINIMUM}KB minimum"
echo ""
echo "Artifacts:"
ls -la "$BUILD_DIR/${FILE_PREFIX}-${VERSION_DISPLAY}."* 2>/dev/null
[ -f "$ABOUT_OUT" ] && echo "  About Geomys included in disk image"
echo ""
echo "Full paths:"
for f in "$BUILD_DIR/${FILE_PREFIX}-${VERSION_DISPLAY}."*; do
    [ -f "$f" ] && echo "  $(readlink -f "$f")"
done

echo ""
echo "To deploy to HFS image:"
echo "  hmount diskimages/snow-sys608.img"
echo "  hmkdir :Geomys"
echo "  hcopy -m build/Geomys.bin ':Geomys:Geomys'"
echo "  hattrib -t APPL -c GEOM ':Geomys:Geomys'"
echo "  hcopy -r 'build/About Geomys' ':Geomys:About Geomys'"
echo "  hattrib -t ttro -c ttxt ':Geomys:About Geomys'"
echo "  humount"
