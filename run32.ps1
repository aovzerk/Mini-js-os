$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "build32.ps1")

if ($LASTEXITCODE) {
    exit $LASTEXITCODE
}

$image = Join-Path $PSScriptRoot "build32\myos32.img"

& qemu-system-i386 `
    -cpu pentium3 `
    -drive "format=raw,file=$image,if=ide" `
    -boot c `
    -display "sdl,show-cursor=off,grab-mod=rctrl"
