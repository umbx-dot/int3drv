#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/bin"
INT3CTL="${BIN_DIR}/int3ctl"
MODULE_KO="${BIN_DIR}/int3drv.ko"

PASS=0
FAIL=0
LIVE=0

pass() { PASS=$((PASS + 1)); echo "pass $*"; }
fail() { FAIL=$((FAIL + 1)); echo "fail $*"; }

run_cmd() {
	if command -v timeout >/dev/null 2>&1; then
		timeout 120 "$@"
	else
		"$@"
	fi
}

if [[ ! -x "${INT3CTL}" || ! -f "${MODULE_KO}" ]]; then
	echo "Build outputs missing. Running build.sh first..."
	"${SCRIPT_DIR}/build.sh"
fi

[[ -x "${INT3CTL}" ]] && pass "int3ctl binary present" || { fail "binary missing"; }
[[ -f "${MODULE_KO}" ]] && pass "int3drv.ko present" || { fail "int3drv.ko missing"; }

if [[ -c "/dev/int3" ]] && "${INT3CTL}" version >/dev/null 2>&1; then
	LIVE=1
	echo "Driver live at /dev/int3."
elif [[ -f "${MODULE_KO}" ]] && sudo -n true >/dev/null 2>&1; then
	echo "/dev/int3 not active. Attempting passwordless insmod..."
	if sudo -n insmod "${MODULE_KO}" >/dev/null 2>&1; then
		for _ in {1..20}; do
			if [[ -c "/dev/int3" ]] && "${INT3CTL}" version >/dev/null 2>&1; then
				break
			fi
			sleep 0.1
		done
	fi
	if [[ -c "/dev/int3" ]] && "${INT3CTL}" version >/dev/null 2>&1; then
		LIVE=1
		echo "Driver loaded and verified."
	else
		echo "note: Driver load unavailable. Running layout-only checks."
	fi
else
	echo "note: /dev/int3 not present and no passwordless sudo. Running layout-only checks."
fi

if run_cmd "${INT3CTL}" selftest; then
	pass "selftest"
else
	fail "selftest (rc=$?)"
fi

if [[ "${LIVE}" -eq 1 ]]; then
	if run_cmd "${INT3CTL}" version >/dev/null 2>&1; then
		pass "version"
	else
		fail "version"
	fi
	if run_cmd "${INT3CTL}" stats >/dev/null 2>&1; then
		pass "stats"
	else
		fail "stats"
	fi
	VER=0
	VER_OUT=$("${INT3CTL}" version 2>/dev/null || true)
	if [[ "${VER_OUT}" =~ ([0-9]+)$ ]]; then
		VER="${BASH_REMATCH[1]}"
	fi
	if [[ "${VER}" -ge 4 ]]; then
		if run_cmd "${INT3CTL}" maps $$ >/dev/null 2>&1; then
			pass "maps"
		else
			fail "maps"
		fi
		if run_cmd "${INT3CTL}" procs >/dev/null 2>&1; then
			pass "procs"
		else
			fail "procs"
		fi
	else
		echo "skip: maps/procs smoke (driver v${VER} < 4)"
	fi
	if [[ "${VER}" -ge 5 ]]; then
		if run_cmd "${INT3CTL}" caps >/dev/null 2>&1; then
			pass "caps"
		else
			fail "caps"
		fi
		if run_cmd "${INT3CTL}" modbase $$ libc >/dev/null 2>&1; then
			pass "modbase"
		else
			fail "modbase"
		fi
	else
		echo "skip: caps/modbase smoke (driver v${VER} < 5)"
	fi
	if [[ "${VER}" -ge 6 ]]; then
		ALLOC_OUT=$(run_cmd "${INT3CTL}" alloc $$ 4096 2>/dev/null || true)
		if [[ "${ALLOC_OUT}" =~ 0x([0-9a-f]+) ]]; then
			pass "alloc"
			if run_cmd "${INT3CTL}" free $$ "0x${BASH_REMATCH[1]}" 4096 >/dev/null 2>&1; then
				pass "free"
			else
				fail "free"
			fi
		else
			fail "alloc"
		fi
	else
		echo "skip: alloc/free smoke (driver v${VER} < 6)"
	fi
else
	echo "skip: live smoke tests (version/stats) — driver not loaded"
fi

echo "${PASS} passed, ${FAIL} failed (live=${LIVE})"
if [[ "${FAIL}" -gt 0 ]]; then
	exit 1
fi
exit 0
