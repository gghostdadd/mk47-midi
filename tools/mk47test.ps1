# Sends note-ons to the MK47 to test host-controlled lighting. Windows
# counterpart of mk47test.swift, using WinMM (the API most MIDI apps use).
# usage: powershell -File mk47test.ps1 sweep | clear | <note> <velocity> [<note> <velocity> ...]
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class WinMM {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    public struct MIDIOUTCAPS {
        public ushort wMid, wPid; public uint vDriverVersion;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)] public string szPname;
        public ushort wTechnology, wVoices, wNotes, wChannelMask; public uint dwSupport;
    }
    [DllImport("winmm.dll")] public static extern uint midiOutGetNumDevs();
    [DllImport("winmm.dll", CharSet = CharSet.Auto)] public static extern uint midiOutGetDevCaps(UIntPtr id, ref MIDIOUTCAPS caps, uint size);
    [DllImport("winmm.dll")] public static extern uint midiOutOpen(out IntPtr handle, uint id, IntPtr cb, IntPtr inst, uint flags);
    [DllImport("winmm.dll")] public static extern uint midiOutShortMsg(IntPtr handle, uint msg);
    [DllImport("winmm.dll")] public static extern uint midiOutClose(IntPtr handle);
}
'@

$dest = -1
for ($i = 0; $i -lt [WinMM]::midiOutGetNumDevs(); ++$i) {
    $caps = New-Object WinMM+MIDIOUTCAPS
    [void][WinMM]::midiOutGetDevCaps([UIntPtr][uint32]$i, [ref]$caps, [Runtime.InteropServices.Marshal]::SizeOf($caps))
    "destination: $($caps.szPname)"
    if ($dest -lt 0 -and $caps.szPname -match 'MK47') { $dest = $i }
}
if ($dest -lt 0) { "MK47 MIDI destination not found"; exit 1 }

$handle = [IntPtr]::Zero
$err = [WinMM]::midiOutOpen([ref]$handle, $dest, [IntPtr]::Zero, [IntPtr]::Zero, 0)
if ($err -ne 0) { "could not open MK47 (MMRESULT $err); another app may have it open"; exit 1 }

function NoteOn([int]$note, [int]$velocity) {
    [void][WinMM]::midiOutShortMsg($handle, [uint32](0x90 -bor ($note -shl 8) -bor ($velocity -shl 16)))
    Start-Sleep -Milliseconds 3
}

if ($args[0] -eq 'sweep') {
    for ($n = 0; $n -lt 128; ++$n) { NoteOn $n $n } # note n shows palette colour n
} elseif ($args[0] -eq 'clear') {
    for ($n = 0; $n -lt 128; ++$n) { NoteOn $n 0 }
} else {
    for ($i = 0; $i + 1 -lt $args.Count; $i += 2) { NoteOn ([int]$args[$i]) ([int]$args[$i + 1]) }
}
Start-Sleep -Milliseconds 200
[void][WinMM]::midiOutClose($handle)
"sent"
