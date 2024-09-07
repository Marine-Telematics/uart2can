import xml.etree.ElementTree as ET
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

            value = "No unit"
            if spn.conversion != "" and spn.unit != "":
                val = eval(spn.conversion, {"x": bit_data})
                value = f"{val} {spn.unit}"

            format += f"    {spn.name.ljust(50)} {bit_data:#0{6}x}  {value}\n"

        print(f"{AnsiColors.OKGREEN}Received data for packet {self.name}\n{AnsiColors.OKCYAN}{format}{AnsiColors.ENDC}")


available_packets: list[J1939Packet] = []


def parse_available() -> None:
    xml_tree = ET.parse("sniffer/packets.xml")
    for packet in xml_tree.getroot():
        name = packet.attrib["name"]
        pgn: int = int(packet.attrib["pgn"], 16)
        spns: list[J1939SPN] = []

        for p_spn in packet.findall('spn'):
            if p_spn is None:
                continue
            if p_spn.tag is None:
                continue

            p_position = p_spn.find("position")
            if p_position is None or not p_position.text:
                continue

            p_lenght = p_spn.find("lenght")
            if p_lenght is None or not p_lenght.text:
                continue

            spn = J1939SPN()
            spn.name = p_spn.attrib["name"]
            spn.byte_index = int(p_position.text.split(".")[0])
            spn.bit_index = int(p_position.text.split(".")[1])
            spn.lenght = int(p_lenght.text)

            p_conversion = p_spn.find("conversion")
            if p_conversion is not None and p_conversion.text:
                spn.conversion = p_conversion.text

            p_unit = p_spn.find("unit")
            if p_unit is not None and p_unit.text:
                spn.unit = p_unit.text

            spns.append(spn)

        available_packets.append(J1939Packet(name, pgn, spns))


def handle_packet(can_packet: CANPacket) -> None:
    identifier = J1939Identifier(can_packet.can_id)

    for packet in available_packets:
        if identifier.pgn == packet.pgn:
            packet.parse_data(can_packet)
            break
