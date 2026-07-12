#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="${1:-release}"

case "$target" in
  release)
    powershell_script="$repo_root/scripts/build-release.ps1"
    ;;
  overlay)
    powershell_script="$repo_root/scripts/build-reversa-dxgi-overlay.ps1"
    ;;
  dxvk)
    powershell_script="$repo_root/scripts/fetch-dxvk-release.ps1"
    ;;
  *)
    printf 'usage: %s [release|overlay|dxvk]\n' "$0" >&2
    exit 2
    ;;
esac

cmd_exe=/mnt/c/Windows/System32/cmd.exe
windows_temp=/mnt/c/Windows/Temp

for command in wslpath mktemp; do
  if ! command -v "$command" >/dev/null 2>&1; then
    printf 'missing required WSL command: %s\n' "$command" >&2
    exit 1
  fi
done

if [[ ! -x "$cmd_exe" ]]; then
  printf 'Windows cmd.exe is unavailable at %s\n' "$cmd_exe" >&2
  exit 1
fi

if [[ ! -f "$powershell_script" ]]; then
  printf 'missing PowerShell build script: %s\n' "$powershell_script" >&2
  exit 1
fi

batch_file="$(mktemp "$windows_temp/bo3-msbuild.XXXXXX.cmd")"
cleanup() {
  rm -f "$batch_file"
}
trap cleanup EXIT

powershell_script_windows="$(wslpath -w "$powershell_script")"
{
  printf '@echo off\r\n'
  printf 'setlocal\r\n'
  printf 'set "TEMP=C:\\Windows\\Temp"\r\n'
  printf 'set "TMP=C:\\Windows\\Temp"\r\n'
  printf 'cd /d C:\\Windows\\Temp\r\n'
  printf 'powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%s"\r\n' "$powershell_script_windows"
  printf 'exit /b %%errorlevel%%\r\n'
} >"$batch_file"

batch_file_windows="$(wslpath -w "$batch_file")"
(
  cd "$windows_temp"
  "$cmd_exe" /d /c "$batch_file_windows"
)
