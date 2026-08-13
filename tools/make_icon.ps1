param(
    [string]$Source,
    [string]$Png
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

function Convert-ToBigEndian([uint32]$Value) {
    return [byte[]]@(
        (($Value -shr 24) -band 255),
        (($Value -shr 16) -band 255),
        (($Value -shr 8) -band 255),
        ($Value -band 255)
    )
}

function Get-Crc32([byte[]]$Buffer) {
    [int64]$crc = 0xFFFFFFFFL

    foreach ($value in $Buffer) {
        $crc = $crc -bxor $value

        for ($bit = 0; $bit -lt 8; ++$bit) {
            if ($crc -band 1) {
                $crc =
                    (($crc -shr 1) -bxor 0xEDB88320L) -band 0xFFFFFFFFL
            } else {
                $crc = $crc -shr 1
            }
        }
    }

    return [uint32]($crc -bxor 0xFFFFFFFFL)
}

function New-PngChunk(
    [string]$Name,
    [byte[]]$Data
) {
    [byte[]]$type = [Text.Encoding]::ASCII.GetBytes($Name)
    [byte[]]$body = $type + $Data
    [byte[]]$length = Convert-ToBigEndian $Data.Length
    [byte[]]$checksum = Convert-ToBigEndian (Get-Crc32 $body)
    $chunk = New-Object Collections.Generic.List[byte]

    $chunk.AddRange($length)
    $chunk.AddRange($body)
    $chunk.AddRange($checksum)

    return $chunk.ToArray()
}

$sourcePath = Resolve-Path $Source
$sourceBitmap = [Drawing.Bitmap]::FromFile($sourcePath)
$cropRectangle = New-Object Drawing.Rectangle 250, 250, 754, 754
$targetRectangle = New-Object Drawing.Rectangle 0, 0, 48, 48
$iconBitmap = New-Object Drawing.Bitmap 48, 48
$graphics = [Drawing.Graphics]::FromImage($iconBitmap)

$graphics.InterpolationMode =
    [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.DrawImage(
    $sourceBitmap,
    $targetRectangle,
    $cropRectangle,
    [Drawing.GraphicsUnit]::Pixel
)

$graphics.Dispose()
$sourceBitmap.Dispose()

$rawPixels = New-Object Collections.Generic.List[byte]

for ($y = 0; $y -lt 48; ++$y) {
    # PNG filter type 0: no filtering for this scanline.
    $rawPixels.Add(0)

    for ($x = 0; $x -lt 48; ++$x) {
        $color = $iconBitmap.GetPixel($x, $y)

        if ($color.R -gt 190 -and
            $color.B -gt 150 -and
            $color.G -lt 100) {
            $alpha = 0
        } else {
            $alpha = 255
        }

        $rawPixels.Add($color.R)
        $rawPixels.Add($color.G)
        $rawPixels.Add($color.B)
        $rawPixels.Add($alpha)
    }
}

$iconBitmap.Dispose()
$pixelBytes = $rawPixels.ToArray()

# Build a zlib stream with one uncompressed DEFLATE block. The GUI decoder
# intentionally supports this small PNG subset.
$zlib = New-Object Collections.Generic.List[byte]
$zlib.Add(0x78)
$zlib.Add(0x01)
$zlib.Add(1)

$blockLength = $pixelBytes.Length
$inverseLength = (-bnot $blockLength) -band 0xFFFF

$zlib.Add($blockLength -band 255)
$zlib.Add(($blockLength -shr 8) -band 255)
$zlib.Add($inverseLength -band 255)
$zlib.Add(($inverseLength -shr 8) -band 255)
$zlib.AddRange($pixelBytes)

[uint32]$adlerA = 1
[uint32]$adlerB = 0

foreach ($value in $pixelBytes) {
    $adlerA = ($adlerA + $value) % 65521
    $adlerB = ($adlerB + $adlerA) % 65521
}

$adlerValue = ($adlerB -shl 16) -bor $adlerA
[byte[]]$adlerBytes = Convert-ToBigEndian $adlerValue
$zlib.AddRange($adlerBytes)

[byte[]]$imageHeader =
    (Convert-ToBigEndian 48) +
    (Convert-ToBigEndian 48) +
    [byte[]]@(8, 6, 0, 0, 0)

$pngBytes = New-Object Collections.Generic.List[byte]
$pngBytes.AddRange([byte[]]@(137, 80, 78, 71, 13, 10, 26, 10))

[byte[]]$headerChunk = New-PngChunk "IHDR" $imageHeader
[byte[]]$dataChunk = New-PngChunk "IDAT" $zlib.ToArray()
[byte[]]$endChunk = New-PngChunk "IEND" ([byte[]]@())

$pngBytes.AddRange($headerChunk)
$pngBytes.AddRange($dataChunk)
$pngBytes.AddRange($endChunk)

[IO.File]::WriteAllBytes($Png, $pngBytes.ToArray())
