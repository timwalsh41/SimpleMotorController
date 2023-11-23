# to run with pytest, simply run from the Script folder:
# python3 -m pytest
#
# Note to get paths to work properly, I also had to use the "terminal.integrated.env.linux" setting in settings.json (this allows us
# to find the modules in Script/Src when running this test file via the integrated terminal (or also by pressing the play button)). To
# get import resolution to work for IntelliSense, I had to add ./Script/Src to "python.analysis.extraPaths" setting.
#
# To get the paths set up for debugging, I had to set up a launch.json with the paths defined. launch.json will define debugging
# configurations, which must be run from the "Run and Debug" view (choose the play symbol with bug on the left side menu). I just took
# the default configuration, and added the path under "env": { "PYTHONPATH": "/x/y/z" }. Then make sure the right configuration is
# selected in the "RUN AND DEBUG" dropdown, and press play to debug.

import Message
import struct

def crc16(data):
    # CRC16 CRC-CCITT polynomial (0x1021)
    assert(type(data) == bytearray or type(data)== bytes)

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


# a sequence of sufficient length, with start sequence, command code,
# valid CRC, and stop sequence; no payload
def testParseValidByteArrayWithoutPayload():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    crc = crc16(data[-1:])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 0)


# a sequence of sufficient length, with start sequence, command code,
# dummy payload, valid CRC, and stop sequence
def testParseValidByteArrayWithPayload():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    crc = crc16(data[len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 0)


# a sequence of sufficient length, with start sequence, command code,
# dummy payload, valid CRC, and stop sequence, along with some extra
# bytes appended at the end that should be returned
def testParseValidByteArrayWithoutPayloadWithExtraBytes():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    crc = crc16(data[-1:])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)
    data.extend([1,2,3,4])

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)

    # we should get a number of bytes back equal to the length
    # of the data appended after the stop sequence
    assert(len(bytes_remaining) == 4)


# a sequence of sufficient length, with command code, payload, CRC, and valid
# stop sequence, but no start sequence
def testMissingStartSequence():
    msg = Message.Message()

    data = bytearray()
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4,5,6,7,8])

    crc = crc16(data[len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_START_SEQUENCE)

    # we should get back the number of bytes in the start sequence, this is what
    # the function should return if we don't match the start sequence (in case
    # there is a partial start sequence in the array that will be completed with
    # the next batch of data appended)
    assert(len(bytes_remaining) == len(Message.CommandInterface.START_SEQUENCE))


# a sequence of sufficient length, with start sequence, command code, payload, 
# and CRC, but no stop sequence; start sequence is a few bytes in from the 
# start of the array
def testMissingStopSequenceWithOffsetStartSequence():
    msg = Message.Message()

    data = bytearray()
    data.extend([1,2,3,4])
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4,5,6,7,8])

    crc = crc16(data[len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_STOP_SEQUENCE)

    # we should get the whole message returned, minus the 4 bytes before the
    # start sequence
    assert(len(bytes_remaining) == len(data)-4)


# a sequence of sufficient length, with start sequence, command code,
# dummy payload, invalid CRC, and stop sequence
def testInvalidCrc():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    # add a dummy invalid CRC
    data.extend([1,2])
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.CRC_CHECK_FAIL)
    assert(len(bytes_remaining) == 0)


# a sequence of insufficient length
def testByteArrayTooShort():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.BYTE_ARRAY_TOO_SHORT)

    # Message does not do any processing on the data if it is too short, e.g. it
    # would not cut off any bytes before a valid start sequence, so we should see
    # the full data array returned
    assert(len(bytes_remaining) == len(data))


# pass in one array containing two valid message sequences (start sequence, command
# code, dummy payload, valid CRC, and stop sequence)
def testTwoFullMessages():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    crc = crc16(data[len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    payload_length = len(data)
    data.extend(data)

    # load and check first message
    error_code, bytes_remaining = msg.loadFromBytes(data)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == payload_length)

    # load and check second message
    error_code, bytes_remaining = msg.loadFromBytes(bytes_remaining)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 0)


# pass in one array containing one valid message sequence (start sequence, command
# code, dummy payload, valid CRC, and stop sequence) and a partial subsequent message;
# combined the returned bytes with the remainder of the second message
def testCombiningMessageAcrossReceptions():
    msg = Message.Message()

    data = bytearray()
    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    crc = crc16(data[len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    payload_length = len(data)
    data.extend(data)

    first_data = data[:(payload_length+5)]
    second_data = data[(payload_length+5):]

    # load and check first message
    error_code, bytes_remaining = msg.loadFromBytes(first_data)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 5)

    # load and check second message
    bytes_remaining.extend(second_data)
    error_code, bytes_remaining = msg.loadFromBytes(bytes_remaining)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 0)


# run a partial message through, then complete it on the next round
def testPartialMessage():
    msg = Message.Message()
    data = bytearray()
    
    # add dummy bytes before start sequence
    data.extend([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15])

    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    # skip the 15 bytes we appended before the start sequence, as well
    # as the start sequence itself, for CRC calculation
    crc = crc16(data[15+len(Message.CommandInterface.START_SEQUENCE):])
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    # cut off the first data in the middle of the start sequence
    first_data = data[:17]
    second_data = data[17:]

    # load and check first message
    error_code, bytes_remaining = msg.loadFromBytes(first_data)

    # we cut off the data in the middle of the start sequence
    assert(error_code == Message.MessageParsingErrorCode.NO_START_SEQUENCE)

    # we should get back the length of the start sequence (including the partial
    # start sequence))
    assert(len(bytes_remaining) == len(Message.CommandInterface.START_SEQUENCE))

    # now append the remaining data bytes to those returned
    bytes_remaining.extend(second_data)
    error_code, bytes_remaining = msg.loadFromBytes(bytes_remaining)
    assert(error_code == Message.MessageParsingErrorCode.NO_ERROR)
    assert(len(bytes_remaining) == 0)


# run a partial message through, then complete it on next round, but with
# an invalid CRC
def testPartialMessageWithError():
    msg = Message.Message()
    data = bytearray()
    
    # add dummy bytes before start sequence
    data.extend([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15])

    data.extend(Message.CommandInterface.START_SEQUENCE)
    data.append(Message.CommandInterface.CMD_GET_STATUS)

    # add a dummy payload
    data.extend([1,2,3,4])

    # here we'll use all the bytes (including the dummy data and start
    # sequence) to get an invalid CRC that should fail once we process
    # the whole message
    crc = crc16(data)
    data.extend(struct.pack('H',crc))
    data.extend(Message.CommandInterface.STOP_SEQUENCE)

    # cut off the first data in the middle of the start sequence
    first_data = data[:17]
    second_data = data[17:]

    # load and check first message
    error_code, bytes_remaining = msg.loadFromBytes(first_data)

    # we cut off the data in the middle of the start sequence
    assert(error_code == Message.MessageParsingErrorCode.NO_START_SEQUENCE)

    # we should get back the length of the start sequence (including the partial
    # start sequence))
    assert(len(bytes_remaining) == len(Message.CommandInterface.START_SEQUENCE))

    # now append the remaining data bytes to those returned
    bytes_remaining.extend(second_data)
    error_code, bytes_remaining = msg.loadFromBytes(bytes_remaining)
    assert(error_code == Message.MessageParsingErrorCode.CRC_CHECK_FAIL)
    assert(len(bytes_remaining) == 0)


# this can be used for debugging
if __name__ == '__main__':
    testPartialMessageWithError()