#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/bin"
INT3CTL="${BIN_DIR}/int3ctl"
MODULE_KO="${BIN_DIR}/int3drv.ko"

if [[ ! -x "${INT3CTL}" || ! -f "${MODULE_KO}" ]]; then
	echo "Driver or tools not found. Running build.sh first..."
	"${SCRIPT_DIR}/build.sh"
fi

driver_is_active() {
	[[ -c "/dev/int3" ]] && "${INT3CTL}" version >/dev/null 2>&1
}

if driver_is_active; then
	echo "int3drv is already loaded and active at /dev/int3."
	"${INT3CTL}" version
else
	echo "Loading int3drv kernel module..."
	sudo insmod "${MODULE_KO}"

	for i in {1..20}; do
		if driver_is_active; then
			break
		fi
		sleep 0.1
	done

	if ! driver_is_active; then
		echo "Failed to verify /dev/int3 after loading module."
		exit 1
	fi

	echo "int3drv loaded and verified successfully. Driver is now active."
	"${INT3CTL}" version
fi

if [[ $# -gt 0 ]]; then
	echo "Executing requested command: $*"
	exec "${INT3CTL}" "$@"
fi

exit 0
