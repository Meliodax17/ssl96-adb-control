<#
    gui.ps1 - Giao dien dieu khien module SSL 96-pixel ADB

    Chay bang:
        powershell -ExecutionPolicy Bypass -File C:\SSL96\tools\gui.ps1

    Hoac bam doi vao  tools\SSL96.cmd

    Khong can nho lenh, khong can go tham so. Moi thu bang nut bam.
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. "$PSScriptRoot\ssl96_hid.ps1"

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

# --------------------------------------------------------------------------
#  Mau sac
# --------------------------------------------------------------------------
$colBg      = [System.Drawing.Color]::FromArgb(245, 246, 248)
$colPanel   = [System.Drawing.Color]::White
$colText    = [System.Drawing.Color]::FromArgb(32, 34, 38)
$colMuted   = [System.Drawing.Color]::FromArgb(110, 116, 124)
$colOk      = [System.Drawing.Color]::FromArgb(34, 160, 84)
$colBad     = [System.Drawing.Color]::FromArgb(206, 56, 56)
$colUnknown = [System.Drawing.Color]::FromArgb(176, 180, 186)
$colAccent  = [System.Drawing.Color]::FromArgb(24, 110, 200)

$fontUi     = New-Object System.Drawing.Font('Segoe UI', 9.5)
$fontBold   = New-Object System.Drawing.Font('Segoe UI', 9.5, [System.Drawing.FontStyle]::Bold)
$fontTitle  = New-Object System.Drawing.Font('Segoe UI', 13, [System.Drawing.FontStyle]::Bold)
$fontMono   = New-Object System.Drawing.Font('Consolas', 9)

# --------------------------------------------------------------------------
#  Cua so chinh
# --------------------------------------------------------------------------
$form                 = New-Object System.Windows.Forms.Form
$form.Text            = 'SSL 96-pixel ADB  -  Bang dieu khien'
$form.Size            = New-Object System.Drawing.Size(940, 720)
$form.StartPosition   = 'CenterScreen'
$form.BackColor       = $colBg
$form.Font            = $fontUi
$form.MinimumSize     = New-Object System.Drawing.Size(860, 640)

$lblTitle          = New-Object System.Windows.Forms.Label
$lblTitle.Text     = 'Module SSL 96-pixel ADB'
$lblTitle.Font     = $fontTitle
$lblTitle.ForeColor= $colText
$lblTitle.Location = New-Object System.Drawing.Point(20, 14)
$lblTitle.AutoSize = $true
$form.Controls.Add($lblTitle)

$lblConn           = New-Object System.Windows.Forms.Label
$lblConn.Text      = 'Chua ket noi'
$lblConn.ForeColor = $colMuted
$lblConn.Location  = New-Object System.Drawing.Point(22, 44)
$lblConn.AutoSize  = $true
$form.Controls.Add($lblConn)

# --------------------------------------------------------------------------
#  Khu vuc trang thai 6 IC
# --------------------------------------------------------------------------
$grpIc          = New-Object System.Windows.Forms.GroupBox
$grpIc.Text     = ' Trang thai 6 IC '
$grpIc.Font     = $fontBold
$grpIc.Location = New-Object System.Drawing.Point(20, 72)
$grpIc.Size     = New-Object System.Drawing.Size(880, 120)
$grpIc.BackColor= $colPanel
$form.Controls.Add($grpIc)

$icNames  = @('IC301','IC302','IC303','IC304','IC305','IC306')
$icRole   = @('master','slave','slave','slave','slave','slave')
$icDots   = @()
$icLabels = @()

for ($i = 0; $i -lt 6; $i++) {
    $x = 20 + ($i * 143)

    $dot            = New-Object System.Windows.Forms.Label
    $dot.Text       = ''
    $dot.BackColor  = $colUnknown
    $dot.Location   = New-Object System.Drawing.Point($x, 34)
    $dot.Size       = New-Object System.Drawing.Size(18, 18)
    $grpIc.Controls.Add($dot)
    $icDots += $dot

    $nm             = New-Object System.Windows.Forms.Label
    $nm.Text        = $icNames[$i]
    $nm.Font        = $fontBold
    $nm.ForeColor   = $colText
    $nm.Location    = New-Object System.Drawing.Point(($x + 24), 34)
    $nm.AutoSize    = $true
    $grpIc.Controls.Add($nm)

    $rl             = New-Object System.Windows.Forms.Label
    $rl.Text        = "dia chi $i, $($icRole[$i])"
    $rl.ForeColor   = $colMuted
    $rl.Location    = New-Object System.Drawing.Point($x, 58)
    $rl.AutoSize    = $true
    $grpIc.Controls.Add($rl)

    $st             = New-Object System.Windows.Forms.Label
    $st.Text        = 'chua ro'
    $st.ForeColor   = $colMuted
    $st.Location    = New-Object System.Drawing.Point($x, 78)
    $st.AutoSize    = $true
    $grpIc.Controls.Add($st)
    $icLabels += $st
}

# --------------------------------------------------------------------------
#  Dieu khien den
# --------------------------------------------------------------------------
$grpLed          = New-Object System.Windows.Forms.GroupBox
$grpLed.Text     = ' Dieu khien den '
$grpLed.Font     = $fontBold
$grpLed.Location = New-Object System.Drawing.Point(20, 202)
$grpLed.Size     = New-Object System.Drawing.Size(880, 196)
$grpLed.BackColor= $colPanel
$form.Controls.Add($grpLed)

# --- Thanh truot do sang ---
$lblLv           = New-Object System.Windows.Forms.Label
$lblLv.Text      = 'Do sang'
$lblLv.Font      = $fontBold
$lblLv.ForeColor = $colText
$lblLv.Location  = New-Object System.Drawing.Point(20, 30)
$lblLv.AutoSize  = $true
$grpLed.Controls.Add($lblLv)

$trk             = New-Object System.Windows.Forms.TrackBar
$trk.Minimum     = 0
$trk.Maximum     = 1023
$trk.Value       = 102
$trk.TickFrequency = 128
$trk.Location    = New-Object System.Drawing.Point(90, 24)
$trk.Size        = New-Object System.Drawing.Size(430, 45)
$grpLed.Controls.Add($trk)

$lblLvVal            = New-Object System.Windows.Forms.Label
$lblLvVal.Font       = $fontBold
$lblLvVal.ForeColor  = $colAccent
$lblLvVal.Location   = New-Object System.Drawing.Point(530, 30)
$lblLvVal.Size       = New-Object System.Drawing.Size(150, 22)
$grpLed.Controls.Add($lblLvVal)

# Canh bao ngan sach dien ap
$lblBudget           = New-Object System.Windows.Forms.Label
$lblBudget.Location  = New-Object System.Drawing.Point(90, 64)
$lblBudget.Size      = New-Object System.Drawing.Size(770, 20)
$grpLed.Controls.Add($lblBudget)

function Update-LevelLabel {
    $w = $trk.Value
    $duty = [math]::Round($w * 100.0 / 1024.0, 1)
    $lblLvVal.Text = "$w / 1023  ($duty%)"

    # ANODE#1 co 4 IC noi tiep = 64 kenh. Bo nguon cho duoi 40V, LED ~3V.
    $sim = [math]::Round(64.0 * $w / 1024.0, 1)
    $v   = [math]::Round($sim * 3.0, 0)
    if ($v -le 40) {
        $lblBudget.ForeColor = $colOk
        $lblBudget.Text = "ANODE#1: ~$sim LED sang dong thoi = ~$v V   (ngan sach 40V - DAT)"
    } else {
        $lblBudget.ForeColor = $colBad
        $lblBudget.Text = "ANODE#1: ~$sim LED sang dong thoi = ~$v V   (VUOT ngan sach 40V - den se toi di)"
    }
}
$trk.Add_ValueChanged({ Update-LevelLabel })

# --- Hang nut chinh ---
function New-Btn {
    param([string]$Text, [int]$X, [int]$Y, [int]$W = 160, [int]$H = 40, [switch]$Primary)
    $b = New-Object System.Windows.Forms.Button
    $b.Text      = $Text
    $b.Location  = New-Object System.Drawing.Point($X, $Y)
    $b.Size      = New-Object System.Drawing.Size($W, $H)
    $b.FlatStyle = 'Flat'
    $b.Font      = $fontUi
    if ($Primary) {
        $b.BackColor = $colAccent
        $b.ForeColor = [System.Drawing.Color]::White
        $b.Font      = $fontBold
    } else {
        $b.BackColor = [System.Drawing.Color]::FromArgb(238, 240, 243)
        $b.ForeColor = $colText
    }
    $b.FlatAppearance.BorderColor = [System.Drawing.Color]::FromArgb(205, 209, 214)
    return $b
}

$btnAll     = New-Btn 'Bat tat ca'        20  96 -Primary
$btnOff     = New-Btn 'Tat het'          190  96
$btnRestore = New-Btn 'Ve bang mac dinh' 360  96 -W 190
$btnDiag    = New-Btn 'Chan doan lai'    560  96 -W 170
$grpLed.Controls.Add($btnAll)
$grpLed.Controls.Add($btnOff)
$grpLed.Controls.Add($btnRestore)
$grpLed.Controls.Add($btnDiag)

# --- Hang nut bat tung IC ---
$lblOnly           = New-Object System.Windows.Forms.Label
$lblOnly.Text      = 'Chi bat mot IC:'
$lblOnly.Font      = $fontBold
$lblOnly.ForeColor = $colText
$lblOnly.Location  = New-Object System.Drawing.Point(20, 150)
$lblOnly.AutoSize  = $true
$grpLed.Controls.Add($lblOnly)

$onlyBtns = @()
for ($i = 0; $i -lt 6; $i++) {
    $b = New-Btn $icNames[$i] (130 + $i * 120) 144 -W 110 -H 32
    $b.Tag = $i
    $grpLed.Controls.Add($b)
    $onlyBtns += $b
}

# --------------------------------------------------------------------------
#  Cua so ket qua
# --------------------------------------------------------------------------
$grpLog          = New-Object System.Windows.Forms.GroupBox
$grpLog.Text     = ' Ket qua '
$grpLog.Font     = $fontBold
$grpLog.Location = New-Object System.Drawing.Point(20, 408)
$grpLog.Size     = New-Object System.Drawing.Size(880, 250)
$grpLog.BackColor= $colPanel
$grpLog.Anchor   = 'Top,Left,Right,Bottom'
$form.Controls.Add($grpLog)

$txt                = New-Object System.Windows.Forms.TextBox
$txt.Multiline      = $true
$txt.ScrollBars     = 'Vertical'
$txt.ReadOnly       = $true
$txt.Font           = $fontMono
$txt.BackColor      = [System.Drawing.Color]::FromArgb(252, 252, 253)
$txt.Location       = New-Object System.Drawing.Point(14, 24)
$txt.Size           = New-Object System.Drawing.Size(852, 214)
$txt.Anchor         = 'Top,Left,Right,Bottom'
$grpLog.Controls.Add($txt)

$lblHint           = New-Object System.Windows.Forms.Label
$lblHint.Text      = 'Meo: keo thanh do sang truoc, roi bam "Bat tat ca". Neu den toi di la da vuot ngan sach dien ap.'
$lblHint.ForeColor = $colMuted
$lblHint.Location  = New-Object System.Drawing.Point(22, 666)
$lblHint.AutoSize  = $true
$lblHint.Anchor    = 'Left,Bottom'
$form.Controls.Add($lblHint)

# --------------------------------------------------------------------------
#  Logic
# --------------------------------------------------------------------------
$script:dev = $null

function Write-Log {
    param([string]$Text)
    $stamp = (Get-Date).ToString('HH:mm:ss')
    $txt.AppendText("[$stamp] $Text`r`n")
    $txt.SelectionStart = $txt.TextLength
    $txt.ScrollToCaret()
}

function Set-Busy {
    param([bool]$On)
    $form.Cursor = if ($On) { 'WaitCursor' } else { 'Default' }
    foreach ($c in @($btnAll, $btnOff, $btnRestore, $btnDiag) + $onlyBtns) { $c.Enabled = -not $On }
    [System.Windows.Forms.Application]::DoEvents()
}

function Ensure-Device {
    if ($null -ne $script:dev) { return $true }
    $script:dev = Open-Ssl96
    if ($null -eq $script:dev) {
        $lblConn.Text      = 'Khong tim thay board tren cong USB'
        $lblConn.ForeColor = $colBad
        return $false
    }
    $lblConn.Text      = "Da ket noi  -  VID 0x0483  PID 0x5750  -  goi $($script:dev.InLen) byte"
    $lblConn.ForeColor = $colOk
    return $true
}

function Invoke-Board {
    param([int]$Cmd, [int[]]$Arguments = @(), [string]$What)

    if (-not (Ensure-Device)) {
        Write-Log "Khong ket noi duoc board. Kiem tra cap USB."
        return $null
    }

    Set-Busy $true
    try {
        $r = Invoke-Ssl96Command -Dev $script:dev -Cmd $Cmd -Arguments $Arguments
    } catch {
        # Board co the vua bi rut ra: dong lai de lan sau mo moi
        Close-Ssl96 $script:dev
        $script:dev = $null
        $lblConn.Text      = 'Mat ket noi'
        $lblConn.ForeColor = $colBad
        Write-Log "Loi khi giao tiep: $($_.Exception.Message)"
        return $null
    } finally {
        Set-Busy $false
    }

    if ($null -eq $r) {
        Write-Log "$What : board khong tra loi"
        return $null
    }
    Write-Log "$What : xong"
    $txt.AppendText($r.Replace("`r`n", "`r`n") + "`r`n")
    $txt.SelectionStart = $txt.TextLength
    $txt.ScrollToCaret()
    return $r
}

<#  Doc hai mat na o dau ban bao cao va to mau 6 cham trang thai.
    Mat na tra loi lenh doc cho biet IC nao noi chuyen duoc;
    mat na ghi that bai cho biet IC nao khong nhan duoc du lieu. #>
function Update-IcStatus {
    param([string]$Report)
    if ([string]::IsNullOrEmpty($Report)) { return }

    $mRead  = [regex]::Match($Report, 'tra loi lenh doc:\s*0x([0-9A-Fa-f]{2})')
    $mWrite = [regex]::Match($Report, 'ghi WIDTH that bai:\s*0x([0-9A-Fa-f]{2})')
    if (-not $mRead.Success -and -not $mWrite.Success) { return }

    $online   = if ($mRead.Success)  { [Convert]::ToInt32($mRead.Groups[1].Value, 16) }  else { 0 }
    $failMask = if ($mWrite.Success) { [Convert]::ToInt32($mWrite.Groups[1].Value, 16) } else { 0 }

    for ($i = 0; $i -lt 6; $i++) {
        $bit      = 1 -shl $i
        $canRead  = ($online   -band $bit) -ne 0
        $canWrite = ($failMask -band $bit) -eq 0

        if ($canRead -and $canWrite) {
            $icDots[$i].BackColor  = $colOk
            $icLabels[$i].Text     = 'tot'
            $icLabels[$i].ForeColor= $colOk
        } elseif ($canWrite) {
            $icDots[$i].BackColor  = [System.Drawing.Color]::FromArgb(230, 160, 30)
            $icLabels[$i].Text     = 'ghi duoc, doc loi'
            $icLabels[$i].ForeColor= [System.Drawing.Color]::FromArgb(180, 120, 20)
        } else {
            $icDots[$i].BackColor  = $colBad
            $icLabels[$i].Text     = 'khong lien lac'
            $icLabels[$i].ForeColor= $colBad
        }
    }
}

# --- Gan hanh dong cho nut ---
$btnDiag.Add_Click({
    $r = Invoke-Board -Cmd $script:SSL96_CMD_RERUN -What 'Chan doan'
    Update-IcStatus $r
})

$btnAll.Add_Click({
    $w = $trk.Value
    $r = Invoke-Board -Cmd $script:SSL96_CMD_ALLLEVEL `
                      -Arguments @(($w -band 0xFF), (($w -shr 8) -band 0xFF)) `
                      -What "Bat tat ca o muc $w"
})

$btnOff.Add_Click({
    $r = Invoke-Board -Cmd $script:SSL96_CMD_ALLLEVEL -Arguments @(0, 0) -What 'Tat het'
})

$btnRestore.Add_Click({
    $r = Invoke-Board -Cmd $script:SSL96_CMD_RESTORE -What 'Ve bang do sang mac dinh'
})

foreach ($b in $onlyBtns) {
    $b.Add_Click({
        $idx = $this.Tag
        $w   = $trk.Value
        Invoke-Board -Cmd $script:SSL96_CMD_ONLY_IC `
                     -Arguments @($idx, ($w -band 0xFF), (($w -shr 8) -band 0xFF)) `
                     -What "Chi bat $($icNames[$idx]) o muc $w"
    })
}

$form.Add_Shown({
    Update-LevelLabel
    Write-Log 'San sang. Bam "Chan doan lai" de doc trang thai 6 IC.'
    if (Ensure-Device) {
        $r = Invoke-Board -Cmd $script:SSL96_CMD_REQUEST -What 'Doc bao cao gan nhat'
        Update-IcStatus $r
    } else {
        Write-Log 'Chua thay board. Cam cap USB roi bam "Chan doan lai".'
    }
})

$form.Add_FormClosed({ Close-Ssl96 $script:dev })

[void]$form.ShowDialog()
