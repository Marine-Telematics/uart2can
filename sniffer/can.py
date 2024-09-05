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
