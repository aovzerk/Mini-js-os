$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build32"

New-Item -ItemType Directory -Force $buildDirectory | Out-Null

function Set-UInt16(
    [byte[]]$Buffer,
    [long]$Offset,
    [uint16]$Value
) {
    $Buffer[$Offset] = $Value -band 255
    $Buffer[$Offset + 1] = ($Value -shr 8) -band 255
}

function Set-UInt32(
    [byte[]]$Buffer,
    [long]$Offset,
    [uint32]$Value
) {
    $Buffer[$Offset] = $Value -band 255
    $Buffer[$Offset + 1] = ($Value -shr 8) -band 255
    $Buffer[$Offset + 2] = ($Value -shr 16) -band 255
    $Buffer[$Offset + 3] = ($Value -shr 24) -band 255
}

function Build-KernelImage(
    [string]$SourceName,
    [string]$OutputName,
    [uint32]$BaseAddress
) {
    $compileCommand =
        "wcc386 -q -s -zl -zq -3r -ox " +
        "-i=pm32\include " +
        "-fo=build32\$OutputName.obj $SourceName"

    cmd /c $compileCommand

    if ($LASTEXITCODE) {
        throw "$SourceName C compile failed"
    }

    $linkOffset = $BaseAddress.ToString("X")
    $linkCommand =
        "wlink format os2 flat option quiet option nodefaultlibs " +
        "option alignment=16 option offset=0x$linkOffset " +
        "option start=kernel_dispatch_ " +
        "file build32\$OutputName.obj " +
        "name build32\$OutputName.lx"

    cmd /c $linkCommand

    if ($LASTEXITCODE) {
        throw "$SourceName link failed"
    }

    $lx = [IO.File]::ReadAllBytes("$buildDirectory\$OutputName.lx")
    $header = [BitConverter]::ToUInt32($lx, 0x3C)
    $dataOffset = [BitConverter]::ToUInt32($lx, $header + 0x80)
    $objectTable =
        $header + [BitConverter]::ToUInt32($lx, $header + 0x40)
    $objectCount = [BitConverter]::ToUInt32($lx, $header + 0x44)
    $pageMap = $header + [BitConverter]::ToUInt32($lx, $header + 0x48)
    $objects = @()
    $imageSize = 0

    for ($objectIndex = 0; $objectIndex -lt $objectCount; ++$objectIndex) {
        $objectOffset = $objectTable + 24 * $objectIndex
        $size = [BitConverter]::ToUInt32($lx, $objectOffset)
        $oldBase = [BitConverter]::ToUInt32($lx, $objectOffset + 4)
        $mapIndex = [BitConverter]::ToUInt32($lx, $objectOffset + 12)
        $pageCount = [BitConverter]::ToUInt32($lx, $objectOffset + 16)
        $storedSize = 0

        for ($page = 0; $page -lt $pageCount; ++$page) {
            $mapOffset = $pageMap + ($mapIndex - 1 + $page) * 8 + 4
            $storedSize += [BitConverter]::ToUInt16($lx, $mapOffset)
        }

        $newBase = ($imageSize + 15) -band (-16)
        $objects += [pscustomobject]@{
            Size = $size
            StoredSize = $storedSize
            OldBase = $oldBase
            NewBase = $newBase
        }
        $imageSize = $newBase + $size
    }

    $rawImage = New-Object byte[] $imageSize
    $sourceOffset = 0

    foreach ($object in $objects) {
        $copySize = [Math]::Min($object.Size, $object.StoredSize)

        if ($copySize) {
            [Array]::Copy(
                $lx,
                $dataOffset + $sourceOffset,
                $rawImage,
                $object.NewBase,
                $copySize
            )
        }
        $sourceOffset += $object.StoredSize
    }

    [IO.File]::WriteAllBytes(
        "$buildDirectory\$OutputName.bin",
        $rawImage
    )
}

function Build-UserProgram([string]$Name) {
    if ($Name -eq "gui" -or $Name -eq "js") {
        $sourceName = "pm32\user\$Name\$Name.c"
    } else {
        $sourceName = "pm32\user\$Name.c"
    }
    $compileCommand =
        "wcc386 -q -s -zl -zq -3r -ox " +
        "-i=pm32\include " +
        "-fo=build32\$Name.obj $sourceName"

    cmd /c $compileCommand

    if ($LASTEXITCODE) {
        throw "$Name C compile failed"
    }

    $linkCommand =
        "wlink format os2 flat option quiet option nodefaultlibs " +
        "option alignment=16 option offset=0x400000 " +
        "option start=_start_ " +
        "file build32\$Name.obj,build32\userlib.obj " +
        "name build32\$Name.lx"

    cmd /c $linkCommand

    if ($LASTEXITCODE) {
        throw "$Name link failed"
    }

    $lx = [IO.File]::ReadAllBytes("$buildDirectory\$Name.lx")
    $header = [BitConverter]::ToUInt32($lx, 0x3C)
    $dataOffset = [BitConverter]::ToUInt32($lx, $header + 0x80)
    $objectTable =
        $header + [BitConverter]::ToUInt32($lx, $header + 0x40)
    $objectCount = [BitConverter]::ToUInt32($lx, $header + 0x44)
    $objects = @()
    $imageSize = 0

    for ($objectIndex = 0; $objectIndex -lt $objectCount; ++$objectIndex) {
        $objectOffset = $objectTable + 24 * $objectIndex
        $size = [BitConverter]::ToUInt32($lx, $objectOffset)
        $oldBase = [BitConverter]::ToUInt32($lx, $objectOffset + 4)
        $newBase = $oldBase - 0x400000

        $objects += [pscustomobject]@{
            Size = $size
            OldBase = $oldBase
            NewBase = $newBase
        }
        $imageSize = $newBase + $size
    }

    $rawImage = New-Object byte[] $imageSize
    $sourceOffset = 0

    foreach ($object in $objects) {
        [Array]::Copy(
            $lx,
            $dataOffset + $sourceOffset,
            $rawImage,
            $object.NewBase,
            $object.Size
        )
        $sourceOffset = ($sourceOffset + $object.Size + 15) -band (-16)
    }

    # Keep WLink's virtual layout (user image begins at 0x400000).
    # Pattern-based relocation is forbidden because it corrupts constants.
    [IO.File]::WriteAllBytes("$buildDirectory\$Name.bin", $rawImage)
}

Push-Location $projectRoot

try {
    & powershell `
        -NoProfile `
        -ExecutionPolicy Bypass `
        -File tools\make_icon.ps1 `
        -Source pm32\assets\shell-icon-source.png `
        -Png pm32\assets\shell-icon.png

    if ($LASTEXITCODE) {
        throw "icon conversion failed"
    }

    Build-KernelImage "pm32\kernel\core\core.c" "kernel-core" 0x20000

    $userLibraryCommand =
        "wcc386 -q -s -zl -zq -3r -ox " +
        "-i=pm32\include " +
        "-fo=build32\userlib.obj pm32\lib\userlib.c"

    cmd /c $userLibraryCommand

    if ($LASTEXITCODE) {
        throw "userlib C compile failed"
    }

    $programNames = @(
        "shell",
        "print",
        "gui",
        "editor",
        "ls",
        "cat",
        "js",
        "poweroff"
    )

    foreach ($programName in $programNames) {
        Build-UserProgram $programName
    }

    & fasm pm32\kernel\entry.asm build32\system.bin

    if ($LASTEXITCODE) {
        throw "system build failed"
    }

    $systemLength = (Get-Item "$buildDirectory\system.bin").Length
    $systemSectors = [int][Math]::Ceiling($systemLength / 512.0)
    $kernelSectors = $systemSectors - 16
    if ($kernelSectors -le 0 -or $kernelSectors -gt 96) {
        throw "SYSTEM kernel part requires $kernelSectors sectors; maximum is 96"
    }

    & fasm "-dKERNEL_SECTORS=$kernelSectors" `
        pm32\boot\boot.asm build32\boot.bin

    if ($LASTEXITCODE) {
        throw "boot build failed"
    }

    $bytesPerSector = 512
    $totalSectors = 131072
    $reservedSectors = 128
    $fatSectors = 1009
    $fatCount = 2
    $boot = [IO.File]::ReadAllBytes("$buildDirectory\boot.bin")
    $system = [IO.File]::ReadAllBytes("$buildDirectory\system.bin")

    if ($system.Length -gt 127 * $bytesPerSector) {
        throw "SYSTEM exceeds reserved area"
    }

    $files = @(
        [pscustomobject]@{
            Name = "SYSTEM  BIN"
            Data = $system
            AppType = 0
        },
        [pscustomobject]@{
            Name = "SHELL   BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\shell.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "PRINT   BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\print.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "GUI     BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\gui.bin")
            AppType = 1
        },
        [pscustomobject]@{
            Name = "EDITOR  BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\editor.bin")
            AppType = 1
        },
        [pscustomobject]@{
            Name = "LS      BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\ls.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "CAT     BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\cat.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "JS      BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\js.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "JSGUI   BIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\js.bin")
            AppType = 1
        },
        [pscustomobject]@{
            Name = "POWEROFFBIN"
            Data = [IO.File]::ReadAllBytes("$buildDirectory\poweroff.bin")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "NOTES   TXT"
            Data = New-Object byte[] 4096
            AppType = 0
        },
        [pscustomobject]@{
            Name = "JSTEST  JS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\jstest.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "APITEST JS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\apitest.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "MONITOR JS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\monitor.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "DESKTOP JS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\desktop.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "DESKAPPSJS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\deskapps.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "WINDOWS JS "
            Data = [IO.File]::ReadAllBytes("pm32\js_scripts\windows.js")
            AppType = 0
        },
        [pscustomobject]@{
            Name = "SHELLICOPNG"
            Data = [IO.File]::ReadAllBytes("pm32\assets\shell-icon.png")
            AppType = 0
        }
    )

    # Clusters 2 and 3 form a contiguous 32-entry root directory.
    $nextCluster = 4

    foreach ($file in $files) {
        $clusterCount =
            [int][Math]::Ceiling($file.Data.Length / $bytesPerSector)

        $file | Add-Member FirstCluster $nextCluster
        $file | Add-Member ClusterCount $clusterCount
        $nextCluster += $clusterCount
    }

    $image = New-Object byte[] ($totalSectors * $bytesPerSector)
    [Array]::Copy($boot, 0, $image, 0, $bytesPerSector)
    [Array]::Copy($system, 0, $image, $bytesPerSector, $system.Length)

    $fat = New-Object byte[] ($fatSectors * $bytesPerSector)
    Set-UInt32 $fat 0 0x0FFFFFF8
    Set-UInt32 $fat 4 ([uint32]::MaxValue)
    Set-UInt32 $fat 8 0x0FFFFFFF
    Set-UInt32 $fat 8 3
    Set-UInt32 $fat 12 0x0FFFFFFF

    foreach ($file in $files) {
        for (
            $clusterIndex = 0;
            $clusterIndex -lt $file.ClusterCount;
            ++$clusterIndex
        ) {
            $cluster = $file.FirstCluster + $clusterIndex

            if ($clusterIndex -eq $file.ClusterCount - 1) {
                $next = 0x0FFFFFFF
            } else {
                $next = $cluster + 1
            }

            Set-UInt32 $fat ($cluster * 4) $next
        }
    }

    for ($fatCopy = 0; $fatCopy -lt $fatCount; ++$fatCopy) {
        $fatOffset =
            ($reservedSectors + $fatCopy * $fatSectors) * $bytesPerSector

        [Array]::Copy($fat, 0, $image, $fatOffset, $fat.Length)
    }

    $dataSector = $reservedSectors + $fatCount * $fatSectors
    $rootOffset = $dataSector * $bytesPerSector

    for ($fileIndex = 0; $fileIndex -lt $files.Count; ++$fileIndex) {
        $file = $files[$fileIndex]
        $entryOffset = $rootOffset + $fileIndex * 32
        $nameBytes = [Text.Encoding]::ASCII.GetBytes($file.Name)
        $fileSector = $dataSector + $file.FirstCluster - 2

        [Array]::Copy($nameBytes, 0, $image, $entryOffset, 11)
        $image[$entryOffset + 11] = 0x20
        $image[$entryOffset + 12] = $file.AppType
        Set-UInt16 $image ($entryOffset + 26) $file.FirstCluster
        Set-UInt32 $image ($entryOffset + 28) $file.Data.Length
        [Array]::Copy(
            $file.Data,
            0,
            $image,
            $fileSector * $bytesPerSector,
            $file.Data.Length
        )
    }

    if ($env:MYOS_IMAGE) {
        $imageName = $env:MYOS_IMAGE
    } else {
        $imageName = "myos32.img"
    }

    $imagePath = Join-Path $buildDirectory $imageName
    [IO.File]::WriteAllBytes($imagePath, $image)

    Write-Host "Built FAT32 build32\$imageName" -ForegroundColor Green
    Write-Host "SYSTEM load: $systemSectors sectors ($kernelSectors kernel sectors)"

    foreach ($file in $files) {
        Write-Host "$($file.Name.Trim()): $($file.Data.Length) bytes"
    }
} finally {
    Pop-Location
}
