#!/usr/bin/env python3

from pathlib import Path
import socket
import time


SERVER = ("127.0.0.1", 27060)
PROBE = ("127.0.0.1", 27061)
DEADLINE_SECONDS = 15


with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
    udp.bind(PROBE)
    udp.settimeout(DEADLINE_SECONDS)
    Path("udp_probe.ready").write_text("ready")

    payload, _ = udp.recvfrom(1200)
    Path("udp_outbound.bin").write_bytes(payload)

    deadline = time.monotonic() + DEADLINE_SECONDS
    while not Path("udp_inbound.ready").exists():
        if time.monotonic() >= deadline:
            raise TimeoutError("timed out waiting for inbound marker")
        time.sleep(0.01)

    udp.sendto(b"7DFPSRCUrs-utils-inbound\x00\xff", SERVER)
