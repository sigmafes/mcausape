# make_nether_icons.ps1
#
# gui/gui_blocks.png is the prerendered icon atlas the inventory and the hotbar
# draw from (ItemRenderer::getAtlasPos -> _mapper). Its 48x48 slots are all taken, so
# the blocks the Nether update added get 16x16 icons in the free small slots at
# the end of the atlas (see fix_gui_icons.ps1 for how the atlas is laid out):
#
#   slot 241  nether quartz ore         (tile 153)
#   slot 242  nether brick fence        (tile 113)  -- the wooden fence's icon, recoloured
#   slot 243  nether brick slab         (tile 182, aux 0)
#   slot 244  quartz slab               (tile 182, aux 1)
#   slot 245  nether wart seeds         (item 372)
#   slots 246-253  monster spawners, one per kind (tile 52, aux 0-7)
#
# Cubes are drawn the way the other icons are: the top face, and two sides that
# are darker, seen from a corner. Safe to run twice.
#
#     powershell -ExecutionPolicy Bypass -File handheld/tools/make_nether_icons.ps1

Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$guiPath  = Join-Path $root 'data\images\gui\gui_blocks.png'
$terrPath = Join-Path $root 'data\images\terrain.png'

$gui  = [System.Drawing.Bitmap]::FromFile($guiPath);  $guiNew = New-Object System.Drawing.Bitmap $gui;  $gui.Dispose()
$trn  = [System.Drawing.Bitmap]::FromFile($terrPath); $terr   = New-Object System.Drawing.Bitmap $trn;  $trn.Dispose()

function Slot-XY($slot) { $j = $slot - 128; return @((($j -band 31) * 16), (432 + [int][Math]::Floor($j / 32) * 16)) }

function Sample($col, $row, $u, $v) {
    $x = [Math]::Min(15, [Math]::Max(0, [int][Math]::Floor($u * 16)))
    $y = [Math]::Min(15, [Math]::Max(0, [int][Math]::Floor($v * 16)))
    return $terr.GetPixel($col * 16 + $x, $row * 16 + $y)
}

function Shade($c, $f) {
    return [System.Drawing.Color]::FromArgb(255,
        [int][Math]::Min(255, $c.R * $f), [int][Math]::Min(255, $c.G * $f), [int][Math]::Min(255, $c.B * $f))
}

# One isometric block, 16x16. $h is how tall it is (1 = a full block, 0.5 = a slab);
# $top / $side are the terrain cells (col,row) of its faces.
function Draw-Cube($slot, $h, $topCell, $sideCell) {
    $o = Slot-XY $slot
    $edge = 8 * $h                    # height of the vertical edges
    $ty = 8 - $edge                   # y of the top vertex: bottom of the cube stays at 16
    # corners: PT top, PL left, PR right, PC centre of the top face (named so they do not clash with $t, $s; PowerShell ignores case)
    $PT = @(8, $ty); $PL = @(0, ($ty + 4)); $PR = @(16, ($ty + 4)); $PC = @(8, ($ty + 8))
    for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) {
        $px = $x + 0.5; $py = $y + 0.5
        $col = [System.Drawing.Color]::FromArgb(0, 0, 0, 0)

        # top face: P = T + s*(R-T) + t*(L-T)
        $ax = $PR[0] - $PT[0]; $ay = $PR[1] - $PT[1]; $bx = $PL[0] - $PT[0]; $by = $PL[1] - $PT[1]
        $det = $ax * $by - $ay * $bx
        $dx = $px - $PT[0]; $dy = $py - $PT[1]
        $s = ($dx * $by - $dy * $bx) / $det
        $t = ($ax * $dy - $ay * $dx) / $det
        if ($s -ge 0 -and $s -le 1 -and $t -ge 0 -and $t -le 1) {
            $col = Shade (Sample $topCell[0] $topCell[1] $s $t) 1.0
        } else {
            # left side: P = L + s*(C-L) + t*(0,edge)
            $s = ($px - $PL[0]) / ($PC[0] - $PL[0])
            $yTop = $PL[1] + $s * ($PC[1] - $PL[1])
            $t = ($py - $yTop) / $edge
            if ($s -ge 0 -and $s -le 1 -and $t -ge 0 -and $t -le 1) {
                $col = Shade (Sample $sideCell[0] $sideCell[1] $s (($t * $h) + (1 - $h))) 0.8
            } else {
                # right side: P = C + s*(R-C) + t*(0,edge)
                $s = ($px - $PC[0]) / ($PR[0] - $PC[0])
                $yTop = $PC[1] + $s * ($PR[1] - $PC[1])
                $t = ($py - $yTop) / $edge
                if ($s -ge 0 -and $s -le 1 -and $t -ge 0 -and $t -le 1) {
                    $col = Shade (Sample $sideCell[0] $sideCell[1] $s (($t * $h) + (1 - $h))) 0.6
                }
            }
        }
        $guiNew.SetPixel($o[0] + $x, $o[1] + $y, $col)
    } }
}

# 241: quartz ore, the same picture on every face
Draw-Cube 241 1.0 @(15, 11) @(15, 11)
# 243: nether brick slab
Draw-Cube 243 0.5 @(0, 14) @(0, 14)
# 244: quartz slab (top face and side face are different cells)
Draw-Cube 244 0.5 @(4, 12) @(4, 13)

# 242: nether brick fence = the wooden fence's icon (48x48, atlas icon 69) shrunk to
# 16x16 and given the colours of nether brick, keeping its light and dark.
$fx = (69 % 10) * 48; $fy = [int][Math]::Floor(69 / 10) * 48
[double]$r = 0; [double]$g = 0; [double]$b = 0; [double]$n = 0
for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) {
    $p = $terr.GetPixel(0 * 16 + $x, 14 * 16 + $y); $r += $p.R; $g += $p.G; $b += $p.B; $n++ } }
$avgR = $r / $n; $avgG = $g / $n; $avgB = $b / $n
[double]$lumSum = 0; [double]$lumN = 0
for ($y = 0; $y -lt 48; $y++) { for ($x = 0; $x -lt 48; $x++) {
    $p = $guiNew.GetPixel($fx + $x, $fy + $y); if ($p.A -eq 0) { continue }
    $lumSum += 0.299 * $p.R + 0.587 * $p.G + 0.114 * $p.B; $lumN++ } }
$lumAvg = $lumSum / $lumN
$o = Slot-XY 242
for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) {
    [double]$sa = 0; [double]$sl = 0; [int]$cnt = 0
    for ($yy = 0; $yy -lt 3; $yy++) { for ($xx = 0; $xx -lt 3; $xx++) {
        $p = $guiNew.GetPixel($fx + $x * 3 + $xx, $fy + $y * 3 + $yy)
        if ($p.A -gt 0) { $sa += $p.A; $sl += 0.299 * $p.R + 0.587 * $p.G + 0.114 * $p.B; $cnt++ } } }
    if ($cnt -lt 5) { $guiNew.SetPixel($o[0] + $x, $o[1] + $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0)); continue }
    $scale = ($sl / $cnt) / $lumAvg
    $guiNew.SetPixel($o[0] + $x, $o[1] + $y, [System.Drawing.Color]::FromArgb(255,
        [int][Math]::Min(255, $avgR * $scale), [int][Math]::Min(255, $avgG * $scale), [int][Math]::Min(255, $avgB * $scale)))
} }

# 245: nether wart (the seed item, tile 372) -- flat, copied from items.png cell (13,7)
$itm = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'data\images\gui\items.png'))
$o = Slot-XY 245
for ($y = 0; $y -lt 16; $y++) { for ($x = 0; $x -lt 16; $x++) { $guiNew.SetPixel($o[0] + $x, $o[1] + $y, $itm.GetPixel(13 * 16 + $x, 7 * 16 + $y)) } }
$itm.Dispose()

# 246..253: the eight monster spawners (tile 52, one per kind), each a cube of its own
# tinted cage: kind 0 is cell (8,5) of terrain.png, kinds 1..7 are row 11, columns 4..10.
for ($kind = 0; $kind -lt 8; $kind++) {
    if ($kind -eq 0) { $cell = @(8, 5) } else { $cell = @((3 + $kind), 11) }
    Draw-Cube (246 + $kind) 1.0 $cell $cell
}

$guiNew.Save($guiPath, [System.Drawing.Imaging.ImageFormat]::Png)
$guiNew.Dispose(); $terr.Dispose()
Write-Host 'Icons 241-244 written. ItemRenderer.cpp must map tile 153 -> 241, 113 -> 242 and tile 182 aux 0/1 -> 243/244.'
