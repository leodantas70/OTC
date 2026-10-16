import ctypes
import socket
import struct
import sys
import time
from pathlib import Path


NPCAP_DLL = r"C:\Windows\System32\Npcap\wpcap.dll"
ERRBUF_SIZE = 256


class PcapIf(ctypes.Structure):
    pass


class PcapAddr(ctypes.Structure):
    pass


PcapIfPtr = ctypes.POINTER(PcapIf)
PcapAddrPtr = ctypes.POINTER(PcapAddr)

PcapIf._fields_ = [
    ("next", PcapIfPtr),
    ("name", ctypes.c_char_p),
    ("description", ctypes.c_char_p),
    ("addresses", PcapAddrPtr),
    ("flags", ctypes.c_uint),
]

PcapAddr._fields_ = [
    ("next", PcapAddrPtr),
    ("addr", ctypes.c_void_p),
    ("netmask", ctypes.c_void_p),
    ("broadaddr", ctypes.c_void_p),
    ("dstaddr", ctypes.c_void_p),
]


class TimeVal(ctypes.Structure):
    _fields_ = [("tv_sec", ctypes.c_long), ("tv_usec", ctypes.c_long)]


class PcapPkthdr(ctypes.Structure):
    _fields_ = [("ts", TimeVal), ("caplen", ctypes.c_uint), ("length", ctypes.c_uint)]


class BpfProgram(ctypes.Structure):
    _fields_ = [("bf_len", ctypes.c_uint), ("bf_insns", ctypes.c_void_p)]


def load_pcap():
    pcap = ctypes.WinDLL(NPCAP_DLL)
    pcap.pcap_findalldevs.argtypes = [ctypes.POINTER(PcapIfPtr), ctypes.c_char_p]
    pcap.pcap_findalldevs.restype = ctypes.c_int
    pcap.pcap_freealldevs.argtypes = [PcapIfPtr]
    pcap.pcap_open_live.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
    pcap.pcap_open_live.restype = ctypes.c_void_p
    pcap.pcap_compile.argtypes = [ctypes.c_void_p, ctypes.POINTER(BpfProgram), ctypes.c_char_p, ctypes.c_int, ctypes.c_uint]
    pcap.pcap_compile.restype = ctypes.c_int
    pcap.pcap_setfilter.argtypes = [ctypes.c_void_p, ctypes.POINTER(BpfProgram)]
    pcap.pcap_setfilter.restype = ctypes.c_int
    pcap.pcap_freecode.argtypes = [ctypes.POINTER(BpfProgram)]
    pcap.pcap_next_ex.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.POINTER(PcapPkthdr)), ctypes.POINTER(ctypes.POINTER(ctypes.c_ubyte))]
    pcap.pcap_next_ex.restype = ctypes.c_int
    pcap.pcap_close.argtypes = [ctypes.c_void_p]
    return pcap


def devices(pcap):
    errbuf = ctypes.create_string_buffer(ERRBUF_SIZE)
    head = PcapIfPtr()
    if pcap.pcap_findalldevs(ctypes.byref(head), errbuf) != 0:
        raise RuntimeError(errbuf.value.decode(errors="replace"))
    result = []
    current = head
    while current:
        entry = current.contents
        result.append((entry.name.decode(errors="replace"), (entry.description or b"").decode(errors="replace"), entry.flags))
        current = entry.next
    pcap.pcap_freealldevs(head)
    return result


def tcp_payload(packet):
    if len(packet) < 14:
        return None
    ether_type = struct.unpack("!H", packet[12:14])[0]
    offset = 14
    if ether_type == 0x8100 and len(packet) >= 18:
        ether_type = struct.unpack("!H", packet[16:18])[0]
        offset = 18
    if ether_type != 0x0800 or len(packet) < offset + 20:
        return None
    ihl = (packet[offset] & 0x0F) * 4
    if packet[offset + 9] != 6 or len(packet) < offset + ihl + 20:
        return None
    src = socket.inet_ntoa(packet[offset + 12:offset + 16])
    dst = socket.inet_ntoa(packet[offset + 16:offset + 20])
    tcp = offset + ihl
    src_port, dst_port = struct.unpack("!HH", packet[tcp:tcp + 4])
    tcp_len = ((packet[tcp + 12] >> 4) & 0x0F) * 4
    payload_offset = tcp + tcp_len
    if payload_offset > len(packet):
        return None
    return src, src_port, dst, dst_port, packet[payload_offset:]


def main():
    pcap = load_pcap()
    found = devices(pcap)
    if len(sys.argv) == 1 or sys.argv[1] == "--list":
        for index, (name, description, flags) in enumerate(found):
            print(f"{index}: {name} | {description} | flags={flags}")
        return 0

    device_index = int(sys.argv[1])
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    output = Path(sys.argv[3]) if len(sys.argv) > 3 else Path("logs/npcap-login.txt")
    name = found[device_index][0]
    errbuf = ctypes.create_string_buffer(ERRBUF_SIZE)
    handle = pcap.pcap_open_live(name.encode(), 65535, 1, 100, errbuf)
    if not handle:
        raise RuntimeError(errbuf.value.decode(errors="replace"))
    program = BpfProgram()
    expression = b"tcp port 7171 and host 35.198.28.170"
    if pcap.pcap_compile(handle, ctypes.byref(program), expression, 1, 0xFFFFFFFF) != 0:
        pcap.pcap_close(handle)
        raise RuntimeError("pcap_compile failed")
    if pcap.pcap_setfilter(handle, ctypes.byref(program)) != 0:
        pcap.pcap_freecode(ctypes.byref(program))
        pcap.pcap_close(handle)
        raise RuntimeError("pcap_setfilter failed")
    pcap.pcap_freecode(ctypes.byref(program))

    lines = [f"device={name}", f"filter={expression.decode()}"]
    deadline = time.monotonic() + duration
    header = ctypes.POINTER(PcapPkthdr)()
    data = ctypes.POINTER(ctypes.c_ubyte)()
    try:
        while time.monotonic() < deadline:
            result = pcap.pcap_next_ex(handle, ctypes.byref(header), ctypes.byref(data))
            if result != 1:
                continue
            raw = ctypes.string_at(data, header.contents.caplen)
            parsed = tcp_payload(raw)
            if not parsed:
                continue
            src, src_port, dst, dst_port, payload = parsed
            if payload:
                lines.append(f"{src}:{src_port} -> {dst}:{dst_port} len={len(payload)} hex={payload.hex(' ')}")
    finally:
        pcap.pcap_close(handle)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"captured_payloads={len(lines) - 2} output={output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
