import CommandInterface
import struct
import logging
from enum import Enum

class MessageParsingErrorCode(Enum):
    NO_ERROR = 0,
    BYTE_ARRAY_TOO_SHORT = 1
    CRC_EXTRACTION_ERROR = 2
    NO_STOP_SEQUENCE = 3
    CRC_CHECK_FAIL = 4
    NO_START_SEQUENCE = 5


class Message:
    def __init__(self, byteArray = None):        
        self._commandCode = None
        self._payload = None
        self._crc = None
        self._logger = logging.getLogger(__name__)

        if byteArray is not None:
            self.loadFromBytes(byteArray)


    def loadFromBytes(self, byteArray):
        if type(byteArray) != bytearray:
            raise TypeError('byteArray must be a byte array')
        
        return self._parse(byteArray)


    def loadFromMessage(self, msg):
        if type(msg) != Message:
            raise TypeError('Incorrect type to load mesage')
        
        self._commandCode = msg._commandCode
        self._payload = msg._payload
        self._crc = msg._crc


    def _parse(self, byteArray):
        N = len(byteArray)
        if N < CommandInterface.MINIMUM_MESSAGE_LENGTH:
            return MessageParsingErrorCode.BYTE_ARRAY_TOO_SHORT, byteArray
                
        startLength = len(CommandInterface.START_SEQUENCE)
        stopLength = len(CommandInterface.STOP_SEQUENCE)
        
        # running index into the byte array
        idx = 0

        # attempt to find start sequence
        foundStartSequence = False
        while (idx + startLength) <= N:
            if byteArray[idx:idx+startLength] == bytearray(CommandInterface.START_SEQUENCE):
                # found start sequence
                foundStartSequence = True
                
                break
            else:
                idx += 1
        
        if not foundStartSequence:
            # we will throw out all but the last bytes (since they may be the
            # beginning of a start sequence)
            return MessageParsingErrorCode.NO_START_SEQUENCE, byteArray[-startLength:]
        
        # store the index of the start sequence, we may need it later if we need to return
        # a partially complete message
        start_sequence_idx = idx

        # move index forward beyond the start sequence
        idx += startLength

        # the index pointer is now at the command code
        self._commandCode = byteArray[idx]

        # now move forward to the first byte of the payload, or the first byte of the
        # CRC if the payload is empty
        idx += 1
        
        # A message requires a start sequence, command code, CRC, and stop sequence; the payload is
        # optional. So we will start looking for the stop sequence at the first place it could occur,
        # which is after the CRC
        stop_sequence_idx = idx + CommandInterface.CRC_LENGTH
        foundStopSequence = False
        while (stop_sequence_idx + stopLength) <= N:
            if byteArray[stop_sequence_idx:stop_sequence_idx+stopLength] == bytearray(CommandInterface.STOP_SEQUENCE):
                # found stop sequence
                foundStopSequence = True
                
                break
            else:
                stop_sequence_idx += 1

        if not foundStopSequence:
            # we will return the byte array starting from the start sequence
            return MessageParsingErrorCode.NO_STOP_SEQUENCE, byteArray[start_sequence_idx:]

        # at this point, we know we have a complete message, so we can now extract payload and CRC
        # the payload length can be found knowing where the stop sequence starts, the length of the
        # CRC, and the location of the first byte after the command code (which is the start of the
        # payload if there is one, or the first byte of the CRC)
        payload_length = stop_sequence_idx - CommandInterface.CRC_LENGTH - idx
        if payload_length > 0:
            self._payload = byteArray[idx:idx+payload_length]
        
        # move the index forward to the start of the CRC (with some protection in case of calculation
        # error)
        idx += max(payload_length, 0)

        # Check if CRC field can be parsed into a 16-bit unsigned integer
        try:
            self._crc = struct.unpack('H', byteArray[idx:idx+CommandInterface.CRC_LENGTH])[0]
        except struct.error:
            # since we cannot extract the CRC, we'll throw out this message            
            return MessageParsingErrorCode.CRC_EXTRACTION_ERROR, byteArray[stop_sequence_idx+stopLength:]

        if not self.check_crc():
            # if CRC check fails, we throw out the message
            return MessageParsingErrorCode.CRC_CHECK_FAIL, byteArray[stop_sequence_idx+stopLength:]

        # if we made it here, message parsing was successful, and we'll return any remaining bytes
        return MessageParsingErrorCode.NO_ERROR, byteArray[stop_sequence_idx+stopLength:]


    def _calculate_crc(self, data):
        # CRC16 CRC-CCITT polynomial (0x1021)
        # I used this site to check CCITT-FALSE calculations:
        # https://crccalc.com/
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
        # return 0x6261


    def check_crc(self):
        data = bytearray([self._commandCode])
        if self._payload is not None:
            data.extend(self._payload)
        calculated_crc = self._calculate_crc(data)

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
        if type(value) == bytearray or type(value) == bytes:
            self._payload = value
            # Recalculate CRC whenever payload changes
            self.set_crc()
        else:
            raise ValueError("Payload must be byte array")
