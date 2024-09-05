import math
from sniffer.ansi import AnsiColors
from sniffer.can import CANPacket


class J1939Identifier:
    priority = 0x00
    pgn = 0x0000
    source_address = 0x00

    def __init__(self, can_id: int) -> None:
        self.source_address = can_id & 0xff
        self.pgn = can_id & 0x3ffff00
        self.priority = (can_id >> 26)


class J1939SPN:
    name = ""
    byte_index = 0
    bit_index = 0
    lenght = 0

    conversion = "str"
    unit = ""


class J1939Packet:
    name: str = ""
    pgn: int = 0x0000
    spns: list[J1939SPN] = []

    def __init__(self, name: str, pgn: int, spns: list[J1939SPN]) -> None:
        self.name = name
        self.pgn = pgn
        self.spns = spns

    def parse_data(self, can_packet: CANPacket) -> None:
        format = ""
        for spn in self.spns:
            used_bytes: int = int(math.ceil(spn.lenght/8))
            data = can_packet.data[spn.byte_index : spn.byte_index + used_bytes]
            data = list(reversed(data))

            raw_bytes: int = 0x00
            for i in range(0, len(data)):
                shift: int = (len(data) - i - 1) * 8
                raw_bytes |= data[i] << shift

            bitmask: int = (1 << spn.lenght) - 1
            bitmask <<= spn.bit_index

            bit_data = (raw_bytes & bitmask) >> spn.bit_index

            value = "No unit for conversion"
            if spn.conversion != "" and spn.unit != "":
                val = eval(spn.conversion, {"x": bit_data})
                value = f"{val} {spn.unit}"

            format += f"    {spn.name.ljust(50)} {bit_data:#0{6}x}  {value}\n"

        print(f"{AnsiColors.OKGREEN}Received data for packet {self.name}\n{AnsiColors.OKCYAN}{format}{AnsiColors.ENDC}")
