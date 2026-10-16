// Sends note-ons to the MK47 to test host-controlled lighting.
// usage: swift mk47test.swift sweep | clear | <note> <velocity> [<note> <velocity> ...]
import CoreMIDI
import Foundation

func name(_ obj: MIDIObjectRef) -> String {
    var s: Unmanaged<CFString>?
    MIDIObjectGetStringProperty(obj, kMIDIPropertyDisplayName, &s)
    return (s?.takeRetainedValue() as String?) ?? "?"
}

var dest: MIDIEndpointRef = 0
for i in 0..<MIDIGetNumberOfDestinations() {
    let d = MIDIGetDestination(i)
    print("destination:", name(d))
    if name(d).contains("MK47") { dest = d }
}
for i in 0..<MIDIGetNumberOfSources() { print("source:", name(MIDIGetSource(i))) }
guard dest != 0 else { print("MK47 MIDI destination not found"); exit(1) }

var client = MIDIClientRef(), port = MIDIPortRef()
MIDIClientCreate("mk47test" as CFString, nil, nil, &client)
MIDIOutputPortCreate(client, "out" as CFString, &port)

func noteOn(_ note: UInt8, _ velocity: UInt8) {
    var list = MIDIPacketList()
    let packet = MIDIPacketListInit(&list)
    _ = MIDIPacketListAdd(&list, 1024, packet, 0, 3, [0x90, note, velocity])
    MIDISend(port, dest, &list)
    usleep(3000)
}

let args = Array(CommandLine.arguments.dropFirst())
if args.first == "sweep" {
    for n in 0..<128 { noteOn(UInt8(n), UInt8(n)) } // note n shows palette colour n
} else if args.first == "clear" {
    for n in 0..<128 { noteOn(UInt8(n), 0) }
} else {
    var i = 0
    while i + 1 < args.count { noteOn(UInt8(args[i])!, UInt8(args[i + 1])!); i += 2 }
}
usleep(200000)
print("sent")
