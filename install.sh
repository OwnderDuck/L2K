#!/bin/bash

# L2K One-click installation script - downloads only two core files
# Usage: sudo ./install.sh [INSTALL_DIR]

set -e

# --- Configuration ---
REPO_BASE="https://raw.githubusercontent.com/OwnderDuck/L2K/main"
FILES=("l2k.cpp" "i18n.h")
DEFAULT_INSTALL_DIR="/usr/local/bin"
# --------------------

# Color output
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

# Check root privileges
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: Please run this script with root privileges (use sudo).${NC}" >&2; exit 1
fi

# Set installation directory
INSTALL_DIR="${1:-$DEFAULT_INSTALL_DIR}"
mkdir -p "$INSTALL_DIR"

# PATH check
if [[ ":$PATH:" != *":$INSTALL_DIR:"* ]]; then
    echo -e "${YELLOW}Warning: Installation directory $INSTALL_DIR is not in PATH${NC}"
fi

echo -e "${GREEN}=== L2K Installation Script Started ===${NC}"
echo -e "Installation directory: ${YELLOW}$INSTALL_DIR${NC}"

# Check compilation dependencies
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}Error: g++ not found. Please install build-essential.${NC}" >&2; exit 1
fi

# Choose download tool
DOWNLOAD_CMD=""
if command -v wget &> /dev/null; then
    DOWNLOAD_CMD="wget -q --show-progress"
elif command -v curl &> /dev/null; then
    DOWNLOAD_CMD="curl -L -# -O"
else
    echo -e "${RED}Error: Neither wget nor curl found. Cannot download.${NC}" >&2
    echo "Please install one of them, e.g.: sudo apt install wget" >&2; exit 1
fi

# Create and enter temporary directory
TMP_DIR=$(mktemp -d)
cd "$TMP_DIR"
echo -e "${YELLOW}Working directory: $TMP_DIR${NC}"

# Download files
echo -e "${YELLOW}Downloading core files...${NC}"
for file in "${FILES[@]}"; do
    url="$REPO_BASE/$file"
    echo -n "  - $file ... "
    if $DOWNLOAD_CMD "$url"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${RED}FAILED${NC}" >&2
        echo "Please check network or URL: $url" >&2; exit 1
    fi
    # Simple verification: check if file is empty
    if [ ! -s "$file" ]; then
        echo -e "${RED}Error: Downloaded $file is empty${NC}" >&2; exit 1
    fi
done

# Compile
echo -e "${YELLOW}Compiling l2k.cpp...${NC}"
if g++ --std=c++17 l2k.cpp -o l2k; then
    echo -e "${GREEN}Compilation successful${NC}"
else
    echo -e "${RED}Compilation failed${NC}" >&2; exit 1
fi

# Install executable
echo -e "${YELLOW}Installing to $INSTALL_DIR...${NC}"
if [ -f "$INSTALL_DIR/l2k" ]; then
    mv "$INSTALL_DIR/l2k" "$INSTALL_DIR/l2k.bak"
    echo "Backed up existing file as l2k.bak"
fi
cp l2k "$INSTALL_DIR/l2k"
chmod 755 "$INSTALL_DIR/l2k"
echo -e "${GREEN}Installation complete${NC}"

# Create and install systemd service file
SERVICE_FILE="/etc/systemd/system/l2k.service"
cat > "$SERVICE_FILE" << EOF
[Unit]
Description=L2K System Monitor
After=multi-user.target

[Service]
Type=simple
ExecStart=$INSTALL_DIR/l2k -f
Restart=always
User=root

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
echo -e "${GREEN}Service file created: $SERVICE_FILE${NC}"
echo "You can manage the service with:"
echo "  sudo systemctl enable l2k  # enable auto-start on boot"
echo "  sudo systemctl start l2k   # start now"
echo "  sudo systemctl status l2k  # check status"

# Cleanup
cd / && rm -rf "$TMP_DIR"

echo -e "${GREEN}=== L2K Installation Successful ===${NC}"
echo "Executable location: $INSTALL_DIR/l2k"
echo "You can run 'sudo l2k' directly to start the program."
