param(
    [string]$OutputPath
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$logRoot = Join-Path $repoRoot "profiles\bo3-vulkan\logs"
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

if (-not $OutputPath) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $OutputPath = Join-Path $logRoot "display-state-$stamp.json"
}

Add-Type -AssemblyName System.Windows.Forms

if (-not ([System.Management.Automation.PSTypeName]"DisplayNative").Type) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class DisplayNative
{
    public const int ENUM_CURRENT_SETTINGS = -1;

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct DISPLAY_DEVICE
    {
        public int cb;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string DeviceName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceString;
        public int StateFlags;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceID;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string DeviceKey;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct DEVMODE
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmDeviceName;
        public short dmSpecVersion;
        public short dmDriverVersion;
        public short dmSize;
        public short dmDriverExtra;
        public int dmFields;
        public int dmPositionX;
        public int dmPositionY;
        public int dmDisplayOrientation;
        public int dmDisplayFixedOutput;
        public short dmColor;
        public short dmDuplex;
        public short dmYResolution;
        public short dmTTOption;
        public short dmCollate;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        public string dmFormName;
        public short dmLogPixels;
        public int dmBitsPerPel;
        public int dmPelsWidth;
        public int dmPelsHeight;
        public int dmDisplayFlags;
        public int dmDisplayFrequency;
        public int dmICMMethod;
        public int dmICMIntent;
        public int dmMediaType;
        public int dmDitherType;
        public int dmReserved1;
        public int dmReserved2;
        public int dmPanningWidth;
        public int dmPanningHeight;
    }

    [DllImport("user32.dll", CharSet = CharSet.Ansi)]
    public static extern bool EnumDisplayDevices(string lpDevice, uint iDevNum, ref DISPLAY_DEVICE lpDisplayDevice, uint dwFlags);

    [DllImport("user32.dll", CharSet = CharSet.Ansi)]
    public static extern bool EnumDisplaySettings(string deviceName, int modeNum, ref DEVMODE devMode);
}
"@
}

function New-DisplayDeviceStruct {
    $device = New-Object DisplayNative+DISPLAY_DEVICE
    $device.cb = [Runtime.InteropServices.Marshal]::SizeOf([type]"DisplayNative+DISPLAY_DEVICE")
    return $device
}

function Get-DisplayMode {
    param([string]$DeviceName)

    $mode = New-Object DisplayNative+DEVMODE
    $mode.dmSize = [Runtime.InteropServices.Marshal]::SizeOf([type]"DisplayNative+DEVMODE")
    $ok = [DisplayNative]::EnumDisplaySettings($DeviceName, [DisplayNative]::ENUM_CURRENT_SETTINGS, [ref]$mode)
    if (-not $ok) {
        return $null
    }

    [pscustomobject]@{
        width = $mode.dmPelsWidth
        height = $mode.dmPelsHeight
        frequency_hz = $mode.dmDisplayFrequency
        bits_per_pixel = $mode.dmBitsPerPel
        position_x = $mode.dmPositionX
        position_y = $mode.dmPositionY
        display_flags = $mode.dmDisplayFlags
        orientation = $mode.dmDisplayOrientation
        fixed_output = $mode.dmDisplayFixedOutput
    }
}

function Decode-MonitorIdString {
    param($Values)

    if (-not $Values) {
        return $null
    }

    return (($Values | Where-Object { $_ -ne 0 } | ForEach-Object { [char]$_ }) -join "")
}

$screenFacts = @(
    [System.Windows.Forms.Screen]::AllScreens |
        ForEach-Object {
            [pscustomobject]@{
                device_name = $_.DeviceName
                primary = $_.Primary
                bounds = [pscustomobject]@{
                    x = $_.Bounds.X
                    y = $_.Bounds.Y
                    width = $_.Bounds.Width
                    height = $_.Bounds.Height
                }
                working_area = [pscustomobject]@{
                    x = $_.WorkingArea.X
                    y = $_.WorkingArea.Y
                    width = $_.WorkingArea.Width
                    height = $_.WorkingArea.Height
                }
                current_mode = Get-DisplayMode -DeviceName $_.DeviceName
            }
        }
)

$wmiMonitors = @(
    Get-CimInstance -Namespace root\wmi -ClassName WmiMonitorID -ErrorAction SilentlyContinue |
        ForEach-Object {
            [pscustomobject]@{
                instance_name = $_.InstanceName
                name = Decode-MonitorIdString -Values $_.UserFriendlyName
                serial = Decode-MonitorIdString -Values $_.SerialNumberID
                manufacturer = Decode-MonitorIdString -Values $_.ManufacturerName
                active = $_.Active
            }
        }
)

$adapters = New-Object System.Collections.Generic.List[object]
for ($i = 0; $true; $i++) {
    $adapter = New-DisplayDeviceStruct
    if (-not [DisplayNative]::EnumDisplayDevices($null, [uint32]$i, [ref]$adapter, 0)) {
        break
    }

    $monitors = New-Object System.Collections.Generic.List[object]
    for ($j = 0; $true; $j++) {
        $monitor = New-DisplayDeviceStruct
        if (-not [DisplayNative]::EnumDisplayDevices($adapter.DeviceName, [uint32]$j, [ref]$monitor, 0)) {
            break
        }

        $monitors.Add([pscustomobject]@{
            device_name = $monitor.DeviceName
            device_string = $monitor.DeviceString
            device_id = $monitor.DeviceID
            device_key = $monitor.DeviceKey
            state_flags = $monitor.StateFlags
        })
    }

    $adapters.Add([pscustomobject]@{
        device_name = $adapter.DeviceName
        device_string = $adapter.DeviceString
        device_id = $adapter.DeviceID
        device_key = $adapter.DeviceKey
        state_flags = $adapter.StateFlags
        current_mode = Get-DisplayMode -DeviceName $adapter.DeviceName
        monitors = $monitors.ToArray()
    })
}

$manifest = [pscustomobject]([ordered]@{
    schema_version = 1
    generated_at = (Get-Date).ToString("o")
    screens = @($screenFacts)
    display_adapters = $adapters.ToArray()
    wmi_monitors = @($wmiMonitors)
})

$outDir = Split-Path -Parent $OutputPath
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$manifest | ConvertTo-Json -Depth 8 | Set-Content -Path $OutputPath -Encoding ascii
Write-Host "Wrote $OutputPath"
