import xml.etree.ElementTree as ET
import serial
import sys

from sniffer import j1939
from sniffer.ansi import AnsiColors
from sniffer.can import CANPacket

TIMEOUT: int = 1
BAUDRATE: int = 115200

def handle_received(data: bytes) -> None:
    packet = CANPacket(data)
    # print(f"received {len(data)} bytes - {[hex(i) for i in data]}")
    if packet.valid:
        j1939.handle_packet(packet)
        # print(f"{AnsiColors.OKGREEN} rcv> can_id {hex(packet.can_id)}  data {packet.data_lenght} {[hex(i) for i in packet.data]}{AnsiColors.ENDC}")


def main() -> None:
    # if len(sys.argv) < 2:
    #     print(f"{AnsiColors.FAIL}Need an argumnt for serial port ie.: sniffer.py PORT <- argument {AnsiColors.ENDC}")
    #     return

    # port = sys.argv[1]
    # dev = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)

    j1939.parse_available()

    test = [0x0C, 0x00, 0x00, 0x21, 0x08]
    data = [0x01, 0x40, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00]
    handle_received(bytes(test+data))

    # while True:
    #     dev.timeout = .01
    #     data = dev.read(16)

    #     if len(data) > 1:
    #         handle_received(data)
    #         dev.flush()

if __name__ == "__main__":
    main()
