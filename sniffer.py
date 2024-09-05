import xml.etree.ElementTree as ET
from time import time as time_now
import serial
import sys

TIMEOUT: int = 1
BAUDRATE: int = 115200

class AnsiColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


class Packet:
    def __init__(self, name: str, data: bytes, frequency: float):
        self.name = name
        self.data = data
        self.frequency = frequency

        self.period = 1 / self.frequency
        self.last_command = 0

    def process(self, dev: serial.Serial) -> None:
        now = time_now()
        delta_t: float = now - self.last_command
        if delta_t > self.period:
            self.last_command = now
            dev.write(self.data)

class CANPacket:
    MIN_DATA_LEN: int = 5
    valid = False

    def __init__(self, data: bytes):
        if len(data) < self.MIN_DATA_LEN:
            self.valid = False
            return

        self.can_id = 0
        self.can_id |= data[0] << 24
        self.can_id |= data[1] << 16
        self.can_id |= data[2] << 8
        self.can_id |= data[3] << 0

        self.data_lenght = int(data[4])
        self.data = data[5:]
        self.valid = True


send_packets: list[Packet] = []


def handle_received(data: bytes) -> None:
    packet = CANPacket(data)
    # print(f"received {len(data)} bytes - {[hex(i) for i in data]}")
    if packet.valid:
        print(f"{AnsiColors.OKGREEN} rcv> can_id {hex(packet.can_id)}  data {packet.data_lenght} {[hex(i) for i in packet.data]}{AnsiColors.ENDC}")


def main() -> None:
    if len(sys.argv) < 2:
        print(f"{AnsiColors.FAIL}Need an argumnt for serial port ie.: sniffer.py PORT <- argument {AnsiColors.ENDC}")
        return

    xml_tree = ET.parse("packets.xml")
    xml_root = xml_tree.getroot()

    for packet in xml_root:
        p_frequency = 0.0
        p_data: list[int] = []

        freq = packet.find(".//frequency[@type='integer']")
        if freq is not None and freq.text:
            p_frequency = float(freq.text)

        iterable = packet.find(".//data[@type='array']")
        if iterable is not None:
            for value in iterable.findall('value'):
                if value.text is not None:
                    p_data.append(int(value.text, 16))

        pack = Packet(packet.tag, bytes(p_data), p_frequency)
        send_packets.append(pack)

        print(f"{AnsiColors.OKCYAN}packet {pack.name}  frequency {p_frequency}Hz  data {[hex(i) for i in p_data]}{AnsiColors.ENDC}")

    port = sys.argv[1]
    dev = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)

    while True:
        for packet in send_packets:
            packet.process(dev)

        dev.timeout = .01
        data = dev.read(16)

        if len(data) > 1:
            handle_received(data)
            dev.flush()

if __name__ == "__main__":
    main()
