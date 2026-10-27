#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
SOURCE_FILE="$PROJECT_DIR/firefly.cpp"
BINARY_FILE="$PROJECT_DIR/firefly"
SERVICE_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
SERVICE_FILE="$SERVICE_DIR/firefly.service"

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Không tìm thấy mã nguồn: $SOURCE_FILE" >&2
    exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
    echo "Chưa cài g++. Cài bằng lệnh: sudo apt install g++" >&2
    exit 1
fi

# Unit file values use systemd's \x20 escape for spaces. Keep the absolute
# paths intact (systemd-escape --path would instead turn them into unit names).
PROJECT_DIR_UNIT="${PROJECT_DIR// /\\x20}"
BINARY_FILE_UNIT="${BINARY_FILE// /\\x20}"

echo "Đang biên dịch Firefly..."
g++ -std=c++17 -O2 "$SOURCE_FILE" -o "$BINARY_FILE"
chmod +x "$BINARY_FILE"

mkdir -p "$SERVICE_DIR"
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Firefly Discord Rich Presence
After=graphical-session.target

[Service]
Type=simple
WorkingDirectory=$PROJECT_DIR_UNIT
ExecStart=$BINARY_FILE_UNIT
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now firefly.service

echo "Firefly đã được bật tự khởi chạy khi bạn đăng nhập Ubuntu."
echo "Trạng thái: systemctl --user status firefly.service"
echo "Nhật ký:    journalctl --user -u firefly.service -f"
echo "Tắt:       systemctl --user disable --now firefly.service"
