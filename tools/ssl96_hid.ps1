<#
    ssl96_hid.ps1 - Lop giao tiep USB HID dung chung

    Khong chay truc tiep. Cac script khac nap file nay bang dau cham:

        . "$PSScriptRoot\ssl96_hid.ps1"

    Board hien ra duoi dang thiet bi HID, VID 0x0483 PID 0x5750, goi 64 byte.
    Windows chen them mot byte Report ID o dau moi goi, nen chieu dai thuc te
    cua goi la 65 byte va noi dung that bat dau tu byte thu hai.
#>

Set-StrictMode -Version Latest

# Ma lenh, phai trung voi tps_diag.h
$script:SSL96_VID          = 0x0483
$script:SSL96_PID          = 0x5750
$script:SSL96_TAG          = 0xD1
$script:SSL96_CMD_REQUEST  = 0xD1
$script:SSL96_CMD_RERUN    = 0xD2
$script:SSL96_CMD_BCAST    = 0xD3
$script:SSL96_CMD_RESTORE  = 0xD4
$script:SSL96_CMD_ONLY_IC  = 0xD5
$script:SSL96_CMD_ALLLEVEL = 0xD6

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

    /*  Truyen bo dem bang IntPtr thay vi bang struct. Cach dung struct voi
        ByValTStr rat de sai kich thuoc khi marshal, va khi sai thi ham chi
        lang le tra ve false, lam danh sach thiet bi rong ma khong bao loi. */
    [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern bool SetupDiGetDeviceInterfaceDetailW(IntPtr set, ref SP_DEVICE_INTERFACE_DATA ifd,
                                                         IntPtr detail, int detailSize,
                                                         ref int required, IntPtr devInfo);

    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern SafeFileHandle CreateFileW(string name, uint access, uint share,
      IntPtr sec, uint disp, uint flags, IntPtr templ);

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

    public static HidDev[] ListAll() {
      var outList = new List<HidDev>();
      foreach (string p in EnumPaths()) {
        HidDev d;
        if (Describe(p, 0, out d)) outList.Add(d);
      }
      return outList.ToArray();
    }

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

<#  Mo board. Tra ve mot doi tuong co Handle, Stream, InLen, OutLen.
    Tra ve $null neu khong tim thay. #>
function Open-Ssl96 {
    $inLen = 0; $outLen = 0
    $h = [Ssl96.Hid]::Open($script:SSL96_VID, $script:SSL96_PID, [ref]$inLen, [ref]$outLen)
    if ($null -eq $h -or $h.IsInvalid) { return $null }

    $fs = New-Object System.IO.FileStream($h, [System.IO.FileAccess]::ReadWrite, [int]$inLen, $false)
    return [pscustomobject]@{
        Handle = $h
        Stream = $fs
        InLen  = [int]$inLen
        OutLen = [int]$outLen
    }
}

function Close-Ssl96 {
    param($Dev)
    if ($null -eq $Dev) { return }
    if ($null -ne $Dev.Stream) { try { $Dev.Stream.Dispose() } catch {} }
    if ($null -ne $Dev.Handle -and -not $Dev.Handle.IsInvalid) { try { $Dev.Handle.Dispose() } catch {} }
}

<#  Gui mot lenh va doc ban bao cao tra ve.
    Cmd la ma lenh; Args la toi da 3 byte tham so (dat vao report[1..3] cua
    firmware). Tra ve chuoi bao cao, hoac $null neu khong nhan duoc gi. #>
function Invoke-Ssl96Command {
    param(
        [Parameter(Mandatory)] $Dev,
        [Parameter(Mandatory)] [int] $Cmd,
        [int[]] $Arguments = @(),
        [int] $TimeoutMs = 8000
    )

    $out = New-Object byte[] $Dev.OutLen
    $out[0] = 0x00                       # Report ID, thiet bi khong dung
    $out[1] = [byte]$Cmd
    for ($i = 0; $i -lt $Arguments.Count -and ($i + 2) -lt $Dev.OutLen; $i++) {
        $out[$i + 2] = [byte]($Arguments[$i] -band 0xFF)
    }

    $Dev.Stream.Write($out, 0, $Dev.OutLen)
    $Dev.Stream.Flush()

    $sb       = New-Object System.Text.StringBuilder
    $total    = 0
    $received = 0
    $deadline = (Get-Date).AddMilliseconds($TimeoutMs)

    while ($true) {
        if ((Get-Date) -gt $deadline) { break }

        $buf  = New-Object byte[] $Dev.InLen
        $task = $Dev.Stream.ReadAsync($buf, 0, $Dev.InLen)
        if (-not $task.Wait(500)) { continue }
        if ($task.Result -le 0) { continue }

        # byte 0 la Report ID, noi dung that bat dau tu byte 1
        if ($buf[1] -ne $script:SSL96_TAG) { continue }

        $idx   = $buf[2]
        $total = $buf[3]
        $cnt   = $buf[4]

        if ($cnt -gt 0) {
            [void]$sb.Append([System.Text.Encoding]::ASCII.GetString($buf, 5, [int]$cnt))
        }
        $received++

        if ($idx -ge ($total - 1)) { break }
    }

    if ($received -eq 0) { return $null }
    return $sb.ToString()
}
