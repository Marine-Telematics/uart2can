import xml.etree.ElementTree as ET

from sniffer.ansi import AnsiColors
from sniffer.can import CANPacket
from sniffer.j1939_pgn import J1939Identifier, J1939Packet, J1939SPN

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
