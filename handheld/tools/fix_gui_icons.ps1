# fix_gui_icons.ps1
#
# gui/gui_blocks.png is the prerendered icon atlas the inventory and the hotbar
# draw from (ItemRenderer::getAtlasPos -> _mapper). Two kinds of holes in it:
#
#   1. Some 16x16 item icons are fully transparent (bow, swords, ingots, sign,
#      gold tools and armour ...). The mapper points at them, so the slot draws
#      as nothing. They are filled here from gui/items.png, which holds the
#      same art, using the (column,row) each item gets in Item.cpp.
#
#   2. Blocks added after the atlas was baked have no mapper entry and fall back
#      to the debug path (a red rectangle). Soul sand gets a 48x48 icon in a free
#      slot, made by tinting the netherrack icon with the average colour of the
#      soul sand cell in terrain.png. ItemRenderer.cpp's _mapper[88] must then
#      be 89.
#
# Safe to run twice: it only writes icons that are still blank.
#
#     powershell -ExecutionPolicy Bypass -File handheld/tools/fix_gui_icons.ps1

Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$root      = Split-Path -Parent $PSScriptRoot
$guiPath   = Join-Path $root 'data\images\gui\gui_blocks.png'
$itemsPath = Join-Path $root 'data\images\gui\items.png'
$terrPath  = Join-Path $root 'data\images\terrain.png'
$rendPath  = Join-Path $root 'src\client\renderer\entity\ItemRenderer.cpp'
$itemCpp   = Join-Path $root 'src\world\item\Item.cpp'

$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$backupDir = Join-Path $env:TEMP "opencode\atlas_backup_$stamp"
New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
Copy-Item $guiPath (Join-Path $backupDir 'gui_blocks.png')
Write-Host "Backup: $backupDir"

$rend = Get-Content $rendPath -Raw
$m = [regex]::Match($rend, '_mapper\[\]\s*=\s*\{([^}]*)\}')
$map = @($m.Groups[1].Value -split ',' | ForEach-Object { [int]($_.Trim()) })

$gui = [System.Drawing.Bitmap]::FromFile($guiPath);   $guiNew = New-Object System.Drawing.Bitmap $gui;   $gui.Dispose()
$itm = [System.Drawing.Bitmap]::FromFile($itemsPath); $items  = New-Object System.Drawing.Bitmap $itm;   $itm.Dispose()
$trn = [System.Drawing.Bitmap]::FromFile($terrPath);  $terr   = New-Object System.Drawing.Bitmap $trn;   $trn.Dispose()

function Count-Opaque($bmp, $x0, $y0, $size) {
    $n = 0
    for ($y = 0; $y -lt $size; $y++) { for ($x = 0; $x -lt $size; $x++) { if ($bmp.GetPixel($x0 + $x, $y0 + $y).A -gt 0) { $n++ } } }
    return $n
}

# item id (256+n) -> source cell in items.png, from "new Xxx(n)...->setIcon(col, row)"
$src = Get-Content $itemCpp -Raw
$cells = @{}
foreach ($mm in [regex]::Matches($src, 'Item::(\w+)\s*=\s*\(?new\s+\w+\((\d+)[^;]*?setIcon\((\d+)\s*,\s*(\d+)\)')) {
    $cells[[int]$mm.Groups[2].Value + 256] = @{ name = $mm.Groups[1].Value; col = [int]$mm.Groups[3].Value; row = [int]$mm.Groups[4].Value }
}

# 1) blank item icons ---------------------------------------------------------
$fixed = 0
for ($id = 256; $id -lt $map.Count; $id++) {
    $v = $map[$id]
    if ($v -lt 128 -or -not $cells.ContainsKey($id)) { continue }
    $j  = $v - 128
    $dx = ($j -band 31) * 16
    $dy = 432 + [int][Math]::Floor($j / 32) * 16
    if ((Count-Opaque $guiNew $dx $dy 16) -ne 0) { continue }

    $c  = $cells[$id]
    $sx = $c.col * 16
    $sy = $c.row * 16
    if ((Count-Opaque $items $sx $sy 16) -eq 0) { continue }
    for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) { $guiNew.SetPixel($dx + $x, $dy + $y, $items.GetPixel($sx + $x, $sy + $y)) } }
    Write-Host ("item {0} {1}: atlas icon {2} <- items.png ({3},{4})" -f $id, $c.name, $v, $c.col, $c.row)
    $fixed++
}
Write-Host "Filled $fixed blank item icons."

# 2) soul sand (tile 88) ------------------------------------------------------
$soulIcon = 89
$netherrackIcon = $map[87]
$dstX = ($soulIcon % 10) * 48
$dstY = [int][Math]::Floor($soulIcon / 10) * 48
if ((Count-Opaque $guiNew $dstX $dstY 48) -eq 0) {
    # average colour of the soul sand cell: terrain.png cell (8,6)
    [long]$r = 0; [long]$g = 0; [long]$b = 0; [long]$n = 0
    for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) {
        $p = $terr.GetPixel(8 * 16 + $x, 6 * 16 + $y)
        if ($p.A -eq 0) { continue }
        $r += $p.R; $g += $p.G; $b += $p.B; $n++
    } }
    $avgR = $r / $n; $avgG = $g / $n; $avgB = $b / $n

    $sx = ($netherrackIcon % 10) * 48
    $sy = [int][Math]::Floor($netherrackIcon / 10) * 48
    [double]$sum = 0; [long]$cnt = 0
    for ($y = 0; $y -lt 48; $y++) { for ($x = 0; $x -lt 48; $x++) {
        $p = $guiNew.GetPixel($sx + $x, $sy + $y); if ($p.A -eq 0) { continue }
        $sum += 0.299 * $p.R + 0.587 * $p.G + 0.114 * $p.B; $cnt++
    } }
    $lumAvg = $sum / $cnt
    for ($y = 0; $y -lt 48; $y++) { for ($x = 0; $x -lt 48; $x++) {
        $p = $guiNew.GetPixel($sx + $x, $sy + $y)
        if ($p.A -eq 0) { $guiNew.SetPixel($dstX + $x, $dstY + $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0)); continue }
        $scale = (0.299 * $p.R + 0.587 * $p.G + 0.114 * $p.B) / $lumAvg
        $nr = [int][Math]::Min(255, [Math]::Max(0, $avgR * $scale))
        $ng = [int][Math]::Min(255, [Math]::Max(0, $avgG * $scale))
        $nb = [int][Math]::Min(255, [Math]::Max(0, $avgB * $scale))
        $guiNew.SetPixel($dstX + $x, $dstY + $y, [System.Drawing.Color]::FromArgb($p.A, $nr, $ng, $nb))
    } }
    Write-Host ("soul sand: atlas icon {0} (from netherrack icon {1}, avg colour {2},{3},{4})" -f $soulIcon, $netherrackIcon, [int]$avgR, [int]$avgG, [int]$avgB)
} else {
    Write-Host "soul sand: icon $soulIcon already present"
}

$guiNew.Save($guiPath, [System.Drawing.Imaging.ImageFormat]::Png)
$guiNew.Dispose(); $items.Dispose(); $terr.Dispose()

# 3) _mapper[88] -> 89 ---------------------------------------------------------
if ($map[88] -ne $soulIcon) {
    $map[88] = $soulIcon
    $text = [IO.File]::ReadAllText($rendPath)
    $newList = ($map -join ', ')
    $text = [regex]::Replace($text, '(_mapper\[\]\s*=\s*\{)[^}]*(\})', { param($mm) $mm.Groups[1].Value + $newList + $mm.Groups[2].Value })
    [IO.File]::WriteAllText($rendPath, $text)
    Write-Host "ItemRenderer.cpp: _mapper[88] = $soulIcon"
}
Write-Host 'Done. Delete handheld/project/web/build/minecraftpe.data before building so the atlas is repacked.'
