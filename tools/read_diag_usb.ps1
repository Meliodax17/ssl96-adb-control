<#
    read_diag_usb.ps1 - Doc bao cao chan doan qua cong USB cua board

    KHONG can ST-LINK, KHONG can mach USB-serial, khong can dau them day nao.
    Chi can cam cap USB cua board vao may tinh nhu binh thuong.

    Cach dung:
        powershell -ExecutionPolicy Bypass -File tools\read_diag_usb.ps1
        powershell -ExecutionPolicy Bypass -File tools\read_diag_usb.ps1 -Rerun
        powershell -ExecutionPolicy Bypass -File tools\read_diag_usb.ps1 -Save bao_cao.txt

    Tham so:
        -Rerun   yeu cau board chay lai phep thu roi moi gui, thay vi lay
                 ban bao cao cu dang luu trong bo nho
        -Save    luu ket qua ra file van ban

    Board hien ra duoi dang thiet bi HID, VID 0x0483 PID 0x5750, goi 64 byte.
#>

[CmdletBinding()]
param(
    [switch]$Rerun,
    [switch]$Broadcast,
    [switch]$Restore,
    [int]$OnlyIc = -1,
    [int]$Level = 0,
    [int]$AllLevel = -1,
    [string]$Save,
    [int]$TimeoutMs = 4000
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Khong dat ten bien la $PID: do la bien tu dong cua PowerShell (so hieu tien trinh)
$VendorId  = 0x0483
$ProductId = 0x5750

$DIAG_TAG         = 0xD1
$DIAG_CMD_REQUEST = 0xD1
$DIAG_CMD_RERUN   = 0xD2

function Write-Step($m) { Write-Host "==> $m" -ForegroundColor Cyan }
function Write-Ok  ($m) { Write-Host "    $m" -ForegroundColor Green }
function Write-Err ($m) { Write-Host "[LOI] $m" -ForegroundColor Red }

# ---------------------------------------------------------------------------
# Lop truy cap HID bang Windows API
# ---------------------------------------------------------------------------
if (-not ('Ssl96.Hid' -as [type])) {
Add-Type -Language CSharp @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace Ssl96 {

  [StructLayout(LayoutKind.Sequential)]
  public struct HIDD_ATTRIBUTES {
    public int Size; public ushort VendorID; public ushort ProductID; public ushort VersionNumber;
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct SP_DEVICE_INTERFACE_DATA {
    public int cbSize; public Guid InterfaceClassGuid; public int Flags; public IntPtr Reserved;
  }

  public class HidDev {
    public string Path;
    public ushort Vid, Pid, InLen, OutLen;
  }

  public static class Hid {
    const int DIGCF_PRESENT = 0x02, DIGCF_DEVICEINTERFACE = 0x10;
    const uint GENERIC_READ = 0x80000000, GENERIC_WRITE = 0x40000000;
    const uint FILE_SHARE_READ = 1, FILE_SHARE_WRITE = 2, OPEN_EXISTING = 3;

    [DllImport("hid.dll")] static extern void HidD_GetHidGuid(out Guid g);
    [DllImport("hid.dll")] static extern bool HidD_GetAttributes(SafeFileHandle h, ref HIDD_ATTRIBUTES a);
    [DllImport("hid.dll")] static extern bool HidD_GetPreparsedData(SafeFileHandle h, out IntPtr p);
    [DllImport("hid.dll")] static extern bool HidD_FreePreparsedData(IntPtr p);
    [DllImport("hid.dll")] static extern int  HidP_GetCaps(IntPtr p, byte[] caps);

    [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr SetupDiGetClassDevsW(ref Guid g, IntPtr enumerator, IntPtr hwnd, int flags);

    [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool SetupDiEnumDeviceInterfaces(IntPtr set, IntPtr devInfo, ref Guid g,
                                                   int index, ref SP_DEVICE_INTERFACE_DATA ifd);

    /* Truyen bo dem bang IntPtr thay vi bang struct. Cach dung struct voi
       ByValTStr rat de sai kich thuoc khi marshal, va khi sai thi ham chi
       lang le tra ve false, lam danh sach thiet bi rong. */
    [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool SetupDiGetDeviceInterfaceDetailW(IntPtr set, ref SP_DEVICE_INTERFACE_DATA ifd,
                                                         IntPtr detail, int detailSize,
                                                         ref int required, IntPtr devInfo);

    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern SafeFileHandle CreateFileW(string name, uint access, uint share,
      IntPtr sec, uint disp, uint flags, IntPtr templ);

    /* Duyet moi giao dien HID va tra ve duong dan thiet bi */
    static List<string> EnumPaths() {
      var paths = new List<string>();
      Guid g; HidD_GetHidGuid(out g);

      IntPtr set = SetupDiGetClassDevsW(ref g, IntPtr.Zero, IntPtr.Zero,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
      if (set == IntPtr.Zero || set == (IntPtr)(-1)) return paths;

      try {
        for (int i = 0; ; i++) {
          var ifd = new SP_DEVICE_INTERFACE_DATA();
          ifd.cbSize = Marshal.SizeOf(typeof(SP_DEVICE_INTERFACE_DATA));
          if (!SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref g, i, ref ifd)) break;

          int need = 0;
          SetupDiGetDeviceInterfaceDetailW(set, ref ifd, IntPtr.Zero, 0, ref need, IntPtr.Zero);
          if (need <= 0) continue;

          IntPtr buf = Marshal.AllocHGlobal(need);
          try {
            /* cbSize cua phan DAU cau truc: 8 tren 64-bit, 6 tren 32-bit
               (4 byte cbSize + 1 ky tu Unicode) */
            Marshal.WriteInt32(buf, (IntPtr.Size == 8) ? 8 : 6);
            if (SetupDiGetDeviceInterfaceDetailW(set, ref ifd, buf, need, ref need, IntPtr.Zero)) {
              IntPtr strPtr = new IntPtr(buf.ToInt64() + 4);
              string p = Marshal.PtrToStringUni(strPtr);
              if (!String.IsNullOrEmpty(p)) paths.Add(p);
            }
          } finally { Marshal.FreeHGlobal(buf); }
        }
      } finally { SetupDiDestroyDeviceInfoList(set); }
      return paths;
    }

    static bool Describe(string path, uint access, out HidDev dev) {
      dev = null;
      var h = CreateFileW(path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
      if (h == null || h.IsInvalid) { if (h != null) h.Dispose(); return false; }
      try {
        var at = new HIDD_ATTRIBUTES();
        at.Size = Marshal.SizeOf(typeof(HIDD_ATTRIBUTES));
        if (!HidD_GetAttributes(h, ref at)) return false;

        var d = new HidDev();
        d.Path = path; d.Vid = at.VendorID; d.Pid = at.ProductID;

        IntPtr pre;
        if (HidD_GetPreparsedData(h, out pre)) {
          byte[] caps = new byte[256];
          HidP_GetCaps(pre, caps);
          /* HIDP_CAPS: Usage(2) UsagePage(2) InputReportByteLength(2) OutputReportByteLength(2) */
          d.InLen  = BitConverter.ToUInt16(caps, 4);
          d.OutLen = BitConverter.ToUInt16(caps, 6);
          HidD_FreePreparsedData(pre);
        }
        dev = d;
        return true;
      } finally { h.Dispose(); }
    }

    /* Liet ke moi thiet bi HID dang cam (chi doc thuoc tinh, khong chiem giu) */
    public static HidDev[] ListAll() {
      var outList = new List<HidDev>();
      foreach (string p in EnumPaths()) {
        HidDev d;
        if (Describe(p, 0, out d)) outList.Add(d);
      }
      return outList.ToArray();
    }

    /* Mo thiet bi dung VID/PID de doc ghi */
    public static SafeFileHandle Open(ushort vid, ushort pid, out ushort inLen, out ushort outLen) {
      inLen = 0; outLen = 0;
      foreach (string p in EnumPaths()) {
        HidDev d;
        if (!Describe(p, 0, out d)) continue;
        if (d.Vid != vid || d.Pid != pid) continue;

        var h = CreateFileW(p, GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
        if (h == null || h.IsInvalid) { if (h != null) h.Dispose(); continue; }
        inLen = d.InLen; outLen = d.OutLen;
        return h;
      }
      return null;
    }
  }
}
'@
}

# ---------------------------------------------------------------------------
Write-Step 'Tim board tren cong USB'
Write-Host ("    VID=0x{0:X4}  PID=0x{1:X4}" -f $VendorId, $ProductId)

$inLen = 0; $outLen = 0
$h = [Ssl96.Hid]::Open($VendorId, $ProductId, [ref]$inLen, [ref]$outLen)

if ($null -eq $h -or $h.IsInvalid) {
    Write-Err 'Khong tim thay board tren cong USB.'
    Write-Host ''
    Write-Host '    Cac thiet bi HID may dang nhin thay:' -ForegroundColor Yellow
    $all = @([Ssl96.Hid]::ListAll())
    if ($all.Count -eq 0) {
        Write-Host '      (khong co thiet bi HID nao)'
    } else {
        $all | ForEach-Object { "VID=0x{0:X4}  PID=0x{1:X4}" -f $_.Vid, $_.Pid } |
            Sort-Object -Unique | ForEach-Object { Write-Host "      $_" }
    }
    Write-Host ''
    Write-Host '    Neu trong danh sach tren CO dong VID=0x0483 PID=0x5750 ma van bao loi,' -ForegroundColor Yellow
    Write-Host '    thi mot phan mem khac (vi du phan mem USB Tool cua Lumissil) dang giu' -ForegroundColor Yellow
    Write-Host '    thiet bi. Hay dong phan mem do roi chay lai.' -ForegroundColor Yellow
    Write-Host ''
    Write-Host '    Neu KHONG co dong do, kiem tra theo thu tu:'
    Write-Host '      1. Cap USB cua board (cong Micro USB CN1) da cam vao may chua'
    Write-Host '      2. Firmware moi da duoc nap chua - ban cu khong co phan chan doan'
    Write-Host '      3. Mo Device Manager, muc Human Interface Devices, rut ra cam lai'
    Write-Host '         xem co thiet bi nao xuat hien roi bien mat khong'
    exit 1
}

Write-Ok ("Da mo duoc board. Goi IN {0} byte, goi OUT {1} byte." -f $inLen, $outLen)

try {
    $fs = New-Object System.IO.FileStream($h, [System.IO.FileAccess]::ReadWrite, [int]$inLen, $false)

    # --- Gui lenh ---------------------------------------------------------
    $cmd = if ($AllLevel -ge 0) { 0xD6 } elseif ($OnlyIc -ge 0) { 0xD5 } elseif ($Broadcast) { 0xD3 } elseif ($Restore) { 0xD4 } elseif ($Rerun) { $DIAG_CMD_RERUN } else { $DIAG_CMD_REQUEST }
    $out = New-Object byte[] $outLen
    $out[0] = 0x00              # Report ID = 0 (thiet bi khong dung report ID)
    $out[1] = [byte]$cmd
    if ($AllLevel -ge 0) {
        $out[2] = [byte]($AllLevel -band 0xFF)
        $out[3] = [byte](($AllLevel -shr 8) -band 0xFF)
    }
    if ($OnlyIc -ge 0) {
        $out[2] = [byte]$OnlyIc
        if ($Level -gt 0) { $out[3] = [byte]($Level -band 0xFF); $out[4] = [byte](($Level -shr 8) -band 0xFF) }
    }
    $fs.Write($out, 0, $outLen)
    $fs.Flush()

    if ($Rerun) { Write-Step 'Da yeu cau board chay lai phep thu, dang cho...' }
    else        { Write-Step 'Da xin ban bao cao, dang doc...' }

    # --- Doc cac goi ------------------------------------------------------
    $sb       = New-Object System.Text.StringBuilder
    $total    = 0
    $received = 0
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)

    while ($true) {
        if ((Get-Date) -gt $deadline) {
            if ($received -eq 0) {
                Write-Err 'Board khong gui goi nao ve trong thoi gian cho.'
                Write-Host '    Rat co the firmware dang chay la ban cu chua co phan chan doan.'
                Write-Host '    Hay build va nap lai, roi chay lai lenh nay.'
                exit 1
            }
            Write-Host ''
            Write-Err ("Chi nhan duoc {0}/{1} goi truoc khi het gio." -f $received, $total)
            break
        }

        $buf  = New-Object byte[] $inLen
        $task = $fs.ReadAsync($buf, 0, [int]$inLen)
        if (-not $task.Wait(500)) { continue }
        $n = $task.Result
        if ($n -le 0) { continue }

        # Byte dau la Report ID, noi dung that bat dau tu byte thu hai
        $off = 1
        if ($buf[$off] -ne $DIAG_TAG) { continue }   # khong phai goi bao cao

        $idx   = $buf[$off + 1]
        $total = $buf[$off + 2]
        $cnt   = $buf[$off + 3]

        if ($cnt -gt 0) {
            $txt = [System.Text.Encoding]::ASCII.GetString($buf, $off + 4, [int]$cnt)
            [void]$sb.Append($txt)
        }
        $received++

        Write-Host ("`r    Da nhan goi {0}/{1}" -f ($idx + 1), $total) -NoNewline

        if ($idx -ge ($total - 1)) { Write-Host ''; break }
    }

    $report = $sb.ToString()
    Write-Host ''
    Write-Host $report

    if ($Save) {
        $report | Out-File -FilePath $Save -Encoding utf8
        Write-Ok "Da luu vao $Save"
    }
}
finally {
    if ($null -ne $h -and -not $h.IsInvalid) { $h.Dispose() }
}

exit 0
