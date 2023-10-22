import CommandInterface
import MotorDriver
import MotorCommanderDataClasses
import logging
import struct
import numpy as np
from datetime import datetime
from Message import Message

# Note we don't implement the commands to get motor PWM drive level and direction since these are returned as
# part of the status byte

# number of milliseconds of current sensor data to request
CURRENT_DATA_MS = 50

# number of individual data points per ms
# see CurrentSensor.h
CURRENT_SENSOR_MEASUREMENTS_PER_MS = 20

class MotorCommander:
    def __init__(self, port):
        self._messageQueue = []
        self._commandLabelMap = {}
        self._driverStatus = MotorCommanderDataClasses.DriverStatus()
        self._motorStatus = MotorCommanderDataClasses.MotorStatus()
        self._buildCommandLabelMap()
        self._initializeLogging()

        self._logger.info('MotorCommander')
        self._logger.info(f'Attempting to connect to motor driver on {port}')

        # open motor driver
        self._motorDriver = MotorDriver.MotorDriver(port)

        self._motorDriver.openPort()
        if not self._motorDriver.isConnected():
            raise MotorCommanderException(f'Unable to open motor driver on {port}')

        # get version
        self.requestVersion()
        returnMessage = Message()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Invalid response from motor driver when reading version')
        self._parseResponse(returnMessage)

        # get status
        self.requestStatus()
        returnMessage = Message()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Invalid response from motor driver when reading status')
        self._parseResponse(returnMessage)
        self._printStatus()


    def _initializeLogging(self):
        self._rootLogger = logging.getLogger()
        self._rootLogger.setLevel(logging.DEBUG)

        handler = logging.StreamHandler()
        handler.setLevel(logging.DEBUG)
        formatter = logging.Formatter('%(asctime)-20s : %(name)-15s : %(levelname)-8s : %(message)s')
        handler.setFormatter(formatter)

        current_datetime = datetime.now().strftime("%Y%m%d-%H%M%S")
        file_handler = logging.FileHandler(f'MotorCommander-{current_datetime}.log')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)

        self._rootLogger.addHandler(handler)
        self._rootLogger.addHandler(file_handler)

        # create a named logger so the MotorCommander log lines are readable and not just
        # tagged with "root"
        self._logger = logging.getLogger(__name__)


    def _sendMessageAndGetResponse(self, outMsg: Message, retMsg: Message):
        self._motorDriver.sendBytes(outMsg.to_byte_array())
        response = self._motorDriver.readResponse()

        if len(response) == 0:
            return False
        
        retMsg.loadFromMessage(response)
        return True


    def tick(self):
        # send messages in queue and process responses
        returnMessage = Message()
        while len(self._messageQueue) > 0:
            if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
                raise MotorCommanderException('Motor driver not responding')
            self._parseResponse(returnMessage)    

        # get status
        self.requestStatus()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Motor driver not responding')
        self._parseResponse(returnMessage)

        # get motor position
        self.requestMotorPosition()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Motor driver not responding')
        self._parseResponse(returnMessage)

        # get motor speed
        self.requestMotorSpeed()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Motor driver not responding')
        self._parseResponse(returnMessage)

        # get motor current
        self.requestMotorCurrent()
        if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
            raise MotorCommanderException('Motor driver not responding')
        self._parseResponse(returnMessage)

        # check for log messages
        while self._driverStatus.IsLogMessageAvailable:
            self.requestLogMessage()
            if not self._sendMessageAndGetResponse(self._messageQueue.pop(), returnMessage):
                raise MotorCommanderException('Motor driver not responding')
            self._parseResponse(returnMessage)


    def _checkStatusByte(self, msg: Message):
        # extract the raw command code from the acked command code
        commandCode = (msg.command_code & ~CommandInterface.ACK_BIT_MASK) & CommandInterface.BYTE_MASK

        # and get the command string
        if commandCode not in self._commandLabelMap.keys():
            raise MotorCommanderException(f'Unknown command code returned by motor driver: 0x{commandCode:x}')

        commandString = self._commandLabelMap[commandCode]

        if msg.payload[0] == CommandInterface.ACK_INVALID_COMMAND:
            raise MotorCommanderException(f'Invalid command received at motor driver: 0x{msg.command_code:x}')
        elif msg.payload[0] == CommandInterface.ACK_INVALID_CRC:
            raise MotorCommanderException(f'Invalid CRC in {commandString} message sent to driver')
        elif msg.payload[0] == CommandInterface.ACK_COMMAND_ERROR:
            raise MotorCommanderException(f'Error processing {commandString} command in motor driver')
        elif msg.payload[0] != CommandInterface.ACK_SUCCESSFUL:
            raise MotorCommanderException(f'Unknown return status code in {commandString} command: {msg.payload[0]}')


    def _parseResponse(self, msg: Message):
        if len(msg.payload) < 1:
            raise MotorCommanderException(f'Empty payload in message returned from driver; command code {msg.command_code}')

        self._checkStatusByte(msg)

        # extract the raw command code from the acked command code
        commandCode = (msg.command_code & ~CommandInterface.ACK_BIT_MASK) & CommandInterface.BYTE_MASK

        if commandCode == CommandInterface.CMD_GET_VERSION:
            self._parseVersion(msg)
        elif commandCode == CommandInterface.CMD_GET_STATUS:
            self._parseStatus(msg)
        elif commandCode == CommandInterface.CMD_GET_MOTOR_POS:
            self._parseMotorPosition(msg)
        elif commandCode == CommandInterface.CMD_GET_MOTOR_SPEED:
            self._parseMotorSpeed(msg)
        elif commandCode == CommandInterface.CMD_GET_MOTOR_CURRENT:
            self._parseMotorCurrent(msg)
        elif commandCode == CommandInterface.CMD_GET_LOG_MESSAGE:
            self._parseLogMessage(msg)
        elif commandCode in CommandInterface.NO_RESPONSE_PAYLOAD_COMMANDS:
            # these commands do not return any data in the payload, only an
            # ack code; if we've made it to this point, then the status byte
            # check in _checkStatusByte was successful, so we know the command
            # completed successfully
            self._logger.info(f'Command {self._commandLabelMap[commandCode]} completed successfully')            
        else:
            raise MotorCommanderException('No handler set up to parse response for command code 0x{commandCode:x}')


    def requestVersion(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_VERSION
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseVersion(self, msg: Message):
        if len(msg.payload) != 4:
            raise ValueError('Invalid payload in version message')
        
        self._logger.info(f'Motor driver version {msg.payload[0]}.{msg.payload[1]}.{msg.payload[2]}')
    
    
    def enableMotor(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_ENABLE_MOTOR
        msg.set_crc()

        self._messageQueue.append(msg)


    def disableMotor(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_DISABLE_MOTOR
        msg.set_crc()

        self._messageQueue.append(msg)


    def sleepMotor(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_SLEEP_DRIVER
        msg.set_crc()

        self._messageQueue.append(msg)


    def wakeMotor(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_WAKE_DRIVER
        msg.set_crc()

        self._messageQueue.append(msg)


    def requestStatus(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_STATUS
        msg.set_crc()

        self._messageQueue.append(msg)

    
    def _parseStatus(self, msg: Message):
        payload = msg.payload
        if len(payload) != 8:
            raise(ValueError('Invalid payload in status message'))

        self._driverStatus = MotorCommanderDataClasses.DriverStatus(msg.payload)


    def requestLogMessage(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_LOG_MESSAGE
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseLogMessage(self, msg: Message):
        payload = msg.payload
        if len(payload) < 3:
            raise(ValueError('Invalid payload in log message'))

        logMessagesRemaining = struct.unpack('H', payload[1:3])

        if logMessagesRemaining == CommandInterface.NO_LOG_MESSAGES_AVAILABLE:
            self._logger.warning('No log messages available')
            return
        elif logMessagesRemaining == 0:
            # we have retrieved the last message available
            self._driverStatus.IsLogMessageAvailable = False

        logTimestamp = struct.unpack('I', payload[3:7])
        messageLength = struct.unpack('H', payload[7:9])
        messageSeverity = payload[9]

        # check that payload length matches the expected message length
        if len(payload) != 10 + messageLength:
            self._logger.error('Received log message does not match expected length')

        logMessage = payload[10:].decode('ascii', 'ignore')
        self._logger.info(f'DRIVER LOG::{logTimestamp}::{CommandInterface.LOG_SEVERITY_MAP[messageSeverity]}::{logMessage}')        


    def rampMotorOn(self, driveLevel: int, rampTime_ms: int, forwardDirection: bool):
        if type(driveLevel) != int:
            raise TypeError('Drive level must be an integer between 0 and 1000 (100%)')
        if driveLevel < 0 or driveLevel > 1000:
            raise ValueError('Drive level must be an integer between 0 and 1000 (100%)')
        
        if type(rampTime_ms) != int:
            raise TypeError('Ramp time (ms) must be an integer of at least 25')
        if rampTime_ms < 25:
            raise ValueError('Ramp time (ms) must be an integer of at least 25')

        if type(forwardDirection) != bool:
            raise TypeError('Forward direction argument must be boolean')
        
        msg = Message()
        msg.command_code = CommandInterface.CMD_RAMP_MOTOR_ON

        msg.payload = struct.pack('H', driveLevel)
        msg.payload += bytes([1]) if forwardDirection else bytes([0])
        msg.payload += struct.pack('H', rampTime_ms)

        msg.set_crc()
        self._messageQueue.append(msg)


    def rampMotorOff(self, rampTime_ms: int):
        if type(rampTime_ms) != int:
            raise TypeError('Ramp time (ms) must be an integer of at least 25')
        if rampTime_ms < 25:
            raise ValueError('Ramp time (ms) must be an integer of at least 25')
        
        msg = Message()
        msg.command_code = CommandInterface.CMD_RAMP_MOTOR_OFF
        msg.payload = struct.pack('H', rampTime_ms)

        msg.set_crc()
        self._messageQueue.append(msg)


    def injectFault(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_INJECT_FAULT
        msg.set_crc()

        self._messageQueue.append(msg)


    def clearFault(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_CLEAR_FAULT
        msg.set_crc()

        self._messageQueue.append(msg)


    def requestMotorPosition(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_MOTOR_POS
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseMotorPosition(self, msg: Message):
        payload = msg.payload
        if len(payload) != 5:
            raise ValueError('Invalid payload in motor position message')

        self._motorStatus.Position = struct.unpack('i', payload[1:5])[0]


    def requestMotorSpeed(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_MOTOR_SPEED
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseMotorSpeed(self, msg: Message):
        payload = msg.payload
        if len(payload) != 5:
            raise ValueError('Invalid payload in motor speed message')

        self._motorStatus.Speed = struct.unpack('f', payload[1:5])[0]


    def requestMotorCurrent(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_MOTOR_CURRENT

        # motor current request payload contains number of ms of data to request
        # we will request 50 ms of data, which is our default tick time
        bytes = struct.pack('H', CURRENT_DATA_MS)
        msg.payload = bytes
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseMotorCurrent(self, msg: Message):
        payload = msg.payload

        # payload length = 1 ack byte + 2 byte number of data points + 4 byte floats for each
        # data point
        payload_len = 1 + 2 + CURRENT_DATA_MS*CURRENT_SENSOR_MEASUREMENTS_PER_MS*4

        if len(payload) != payload_len:
            raise ValueError('Invalid payload length in motor speed message')
        
        num_samples = struct.unpack('H', msg.payload[1:3])
        if num_samples != CURRENT_DATA_MS*CURRENT_SENSOR_MEASUREMENTS_PER_MS:
            raise ValueError('Number of current samples returned does not match expected value')

        unpackFormatString = f'{num_samples}f'
        currentMeasurementArray = struct.unpack(unpackFormatString, msg.payload[3:])
        self._motorStatus.Current = np.mean(currentMeasurementArray)


    def setMaxDutyCycle(self, dutyCycle: int):
        if type(dutyCycle) != int:
            raise TypeError('Duty cycle must be an integer')
        
        if dutyCycle < 0 or dutyCycle > 1000:
            raise ValueError('Duty cycle must be an integer between 0 and 1000 (100%)')
        
        msg = Message()
        msg.command_code = CommandInterface.CMD_SET_MAX_DC
        msg.payload = struct.pack('H', dutyCycle)
        msg.set_crc()

        self._messageQueue.append(msg)


    def _printStatus(self):
        self._logger.info('Motor driver status:')
        self._logger.info(f'\tState:{MotorCommanderDataClasses.MotorStatusLabel[self._driverStatus.ControllerState]}')
        self._logger.info(f'\tDirection: {"forward" if status.ForwardDirection else "reverse"}')
        self._logger.info(f'\tSleeping: {self._driverStatus.IsSleeping}')
        self._logger.info(f'\tFaulted: {self._driverStatus.IsFaulted}')
        self._logger.info(f'\tLog messages available? {self._driverStatus.IsLogMessageAvailable}')
        self._logger.info(f'\tMotor drive level: {self._driverStatus.DriveLevel}')
        self._logger.info(f'\tTime in state: {self._driverStatus.TimeInState_ms} ms')

    
    def _buildCommandLabelMap(self):
        self._commandLabelMap[CommandInterface.ACK_SUCCESSFUL] = "Command executed successfully"
        self._commandLabelMap[CommandInterface.ACK_INVALID_CRC] = "Command failure; invalid CRC"
        self._commandLabelMap[CommandInterface.ACK_INVALID_COMMAND] = "Command failure: invalid command code"
        self._commandLabelMap[CommandInterface.ACK_COMMAND_ERROR] = "Command failure: error processing command"

        self._commandLabelMap[CommandInterface.CMD_ENABLE_MOTOR] = "Enable motor"
        self._commandLabelMap[CommandInterface.CMD_DISABLE_MOTOR] = "Disable motor"
        self._commandLabelMap[CommandInterface.CMD_SLEEP_DRIVER] = "Sleep motor"
        self._commandLabelMap[CommandInterface.CMD_WAKE_DRIVER] = "Wake motor"
        self._commandLabelMap[CommandInterface.CMD_CLEAR_FAULT] = "Clear fault"

        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_POS] = "Get motor position"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_PWM] = "Get motor PWM"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_SPEED] = "Get motor speed"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_DIR] = "Get motor direction"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_CURRENT] = "Get motor current"
        
        self._commandLabelMap[CommandInterface.CMD_RAMP_MOTOR_ON] = "Ramp motor on"
        self._commandLabelMap[CommandInterface.CMD_RAMP_MOTOR_OFF] = "Ramp motor off"
        self._commandLabelMap[CommandInterface.CMD_SET_MAX_DC] = "Set maximum DC"

        self._commandLabelMap[CommandInterface.CMD_INJECT_FAULT] = "Inject fault"
        self._commandLabelMap[CommandInterface.CMD_GET_VERSION] = "Get driver version"
        self._commandLabelMap[CommandInterface.CMD_GET_STATUS] = "Get driver status"
        self._commandLabelMap[CommandInterface.CMD_GET_LOG_MESSAGE] = "Get log message"


    def getDriverStatus(self):
        return self._driverStatus


class MotorCommanderException(Exception):
    pass