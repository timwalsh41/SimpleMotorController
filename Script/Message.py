import struct
import CommandInterface
import logging

class Message:
    def __init__(self, byteArray = None):        
        self._startCode = None
        self._commandCode = None
        self._payload = None
        self._crc = None
        self._stopCode = None
        self._logger = logging.getLogger(__name__)

        if byteArray is not None:
            self._byteArray = byteArray
            self._parse()


    def loadFromMessage(self, msg):
        if type(msg) != Message:
            raise TypeError('Incorrect type to load mesage')
        self._startCode = msg._startCode
        self._commandCode = msg._commandCode
        self._payload = msg._payload
        self._stopCode = msg._stopCode


    def _parse(self):
        N = len(self._byteArray)
        if N < CommandInterface.MINIMUM_MESSAGE_LENGTH:
            raise ValueError('Invalid byte array (shorter than minimum length)')
        
        # start sequence marker
        startLength = len(CommandInterface.START_SEQUENCE)
        stopLength = len(CommandInterface.STOP_SEQUENCE)
        self._startCode = self._byteArray[0:startLength]
        
        # Check if start sequence matches CommandInterface.START_SEQUENCE
        if self._startCode != CommandInterface.START_SEQUENCE:
            raise ValueError('Start sequence does not match')

        self._commandCode = self._byteArray[startLength]
        self._payload = self._byteArray[startLength+1:N-stopLength-2]
        
        # Check if CRC field can be parsed into a 16-bit unsigned integer
        try:
            self._crc = struct.unpack('H', self._byteArray[N-stopLength-2:N-stopLength])[0]
        except struct.error:
            raise ValueError('Error extracting CRC')
        
        self._stopCode = self._byteArray[N-stopLength:N]

        # Check if stop sequence matches CommandInterface.STOP_SEQUENCE
        if self._stopCode != CommandInterface.STOP_SEQUENCE:
            raise ValueError('Stop sequence does not match')

        if not self.check_crc():
            self._logger.error('Message failed CRC check')


    def _calculate_crc(self, data):
        # CRC16 CRC-CCITT polynomial (0x1021)
        crc = 0xFFFF
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc <<= 1
                crc &= 0xFFFF
        return crc


    def check_crc(self):
        calculated_crc = self.calculate_crc(self._commandCode + self._payload)
        return calculated_crc == self._crc


    def set_crc(self):
      """Calculate and set the CRC based on the current command code and payload."""
      data_to_crc_check = [self.command_code]
      if self.payload is not None:
          data_to_crc_check += list(self.payload)
      self.crc = self._calculate_crc(data_to_crc_check)


    def to_byte_array(self):
      """Create a byte array from the message."""
      # Start with the start sequence.
      byte_array = bytearray(CommandInterface.START_SEQUENCE)
      # Add the command code.
      byte_array.append(self.command_code)
      # Add the payload.
      if self.payload is not None:
        byte_array.extend(self.payload)
      # Add the CRC.
      byte_array.extend(struct.pack('H',self.crc))
      # Add the stop sequence.
      byte_array.extend(CommandInterface.STOP_SEQUENCE)
      
      return byte_array


    # Getter and Setter for command code with error checking
    @property
    def command_code(self):
        return self._commandCode

    @command_code.setter
    def command_code(self, value):
        if isinstance(value, int) and 0 <= value <= 255:
            self._commandCode = value
            # Recalculate CRC whenever command code changes
            self.set_crc()
        else:
            raise ValueError("Command code must be a single byte (integer between 0 and 255)")

    # Getter and Setter for payload with error checking
    @property
    def payload(self):
        return self._payload

    @payload.setter
    def payload(self, value):
        if isinstance(value, list) and all(isinstance(i, int) and 0 <= i <= 255 for i in value):
            self._payload = value
            # Recalculate CRC whenever payload changes
            self.set_crc()
        else:
            raise ValueError("Payload must be an array of bytes (integers between 0 and 255)")
