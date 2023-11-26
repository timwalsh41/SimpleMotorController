import CommandInterface
import MotorDriver
import MotorCommanderDataClasses
import logging
import struct
import time
import signal
import sys
from InfiniteTimer.InfiniteTimer import InfiniteTimer
import numpy as np
from datetime import datetime
from collections import deque
from Message import Message, MessageParsingErrorCode

# Note we don't implement the commands to get motor PWM drive level and direction since these are returned as
# part of the status byte

# number of milliseconds of current sensor data to request
CURRENT_DATA_MS = 50

# number of individual data points per ms
# see CurrentSensor.h
CURRENT_SENSOR_MEASUREMENTS_PER_MS = 20

# use daemonize=True to have the MotorCommander object run the tick in the background
# at a 10 Hz rate
class MotorCommander:
    def __init__(self, port, daemonize=False, tickTime_s=0.1):
        if type(tickTime_s) not in (int, float):
            raise TypeError('tickTime_s must be a numeric value greater than zero')
        
        if tickTime_s <= 0:
            raise TypeError('tickTime_s must be a numeric value greater than zero')
        
        if type(daemonize) is not bool:
            raise TypeError('daemonize must be true or false')

        self._messageQueue = deque()
        self._commandLabelMap = {}
        self._messageBytes = bytearray()

        self._daemonize = daemonize

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

        # read and throw away any build up data
        self._motorDriver.readBytes()

        # get version - this will print results below from the main tick loop
        self.requestVersion()

        if self._daemonize:
            self._tickTimer = InfiniteTimer(tickTime_s, self.tick)
            self._tickTimer.start()
            signal.signal(signal.SIGINT, self._sigintHandler)
            

    def _sigintHandler(self, sig, frame):
        self._tickTimer.cancel()
        self._logger.fatal('Received SIGINT, terminating')
        sys.exit(0)


    def stop(self):
        if self._daemonize:
            self._tickTimer.cancel()


    def _initializeLogging(self):
        self._rootLogger = logging.getLogger()
        self._rootLogger.setLevel(logging.DEBUG)

        self._streamHandler = logging.StreamHandler()
        self._streamHandler.setLevel(logging.DEBUG)
        self._streamHandler.terminator = '\r\n'
        formatter = logging.Formatter('%(asctime)-20s : %(name)-15s : %(levelname)-8s : %(message)s')
        self._streamHandler.setFormatter(formatter)

        current_datetime = datetime.now().strftime("%Y%m%d-%H%M%S")
        file_handler = logging.FileHandler(f'MotorCommander-{current_datetime}.log')
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)

        self._rootLogger.addHandler(self._streamHandler)
        self._rootLogger.addHandler(file_handler)

        # create a named logger so the MotorCommander log lines are readable and not just
        # tagged with "root"
        self._logger = logging.getLogger(__name__)


    def _sendMessageAndReadResponse(self):
        if len(self._messageQueue) > 0:
            self._motorDriver.sendBytes(self._messageQueue.popleft().to_byte_array())
            
        # read any available response from the driver
        return self._motorDriver.readBytes()


    def tick(self):
        continueFlag = True

        # standard commands to run each iteration of the tick loop
        # these function calls will add messages to the command queue that will
        # then be transmitted in the loop below 
        self.requestStatus()
        self.requestMotorPosition()
        self.requestMotorSpeed()
        self.requestMeanCurrent()

        while continueFlag:
            # send messages in queue            
            response = self._sendMessageAndReadResponse()

            if len(response) == 0:
                # if we're not getting responses, then assume we can stop
                # (this is overrided below if there are more messages to transmit)
                continueFlag = False
            else:
                self._messageBytes.extend(response)
                msg = Message()
                messageParseSuccess = MessageParsingErrorCode.NO_ERROR
                while messageParseSuccess == MessageParsingErrorCode.NO_ERROR and len(self._messageBytes) > 0:                    
                    messageParseSuccess, self._messageBytes = msg.loadFromBytes(self._messageBytes)

                    if messageParseSuccess == MessageParsingErrorCode.NO_ERROR:
                        self._parseResponse(msg)
                    else:
                        if msg.command_code is not None:
                            self._logger.error(f'Error parsing {self._commandLabelMap[msg.command_code & ~CommandInterface.ACK_BIT_MASK]} message: {messageParseSuccess.name}')
                        else:
                            self._logger.error(f'Error parsing message: {messageParseSuccess.name}')
            
            if len(self._messageQueue) > 0:
                continueFlag = True

            time.sleep(0.001)


    def _checkStatusByte(self, msg: Message):
        # extract the raw command code from the acked command code
        commandCode = (msg.command_code & ~CommandInterface.ACK_BIT_MASK) & CommandInterface.BYTE_MASK

        # and get the command string
        if commandCode not in self._commandLabelMap.keys():
            self._logger.error(f'Unknown command code returned by motor driver: 0x{commandCode:02x}')
            return False

        commandString = self._commandLabelMap[commandCode]

        if msg.payload[0] == CommandInterface.ACK_INVALID_COMMAND:
            self._logger.error(f'Invalid command received at motor driver: 0x{msg.command_code:02x}')
            return False
        elif msg.payload[0] == CommandInterface.ACK_INVALID_CRC:
            self._logger.error(f'Invalid CRC in {commandString} message sent to driver')
            return False
        elif msg.payload[0] == CommandInterface.ACK_COMMAND_ERROR:
            self._logger.error(f'Error processing {commandString} command in motor driver')
            return False
        elif msg.payload[0] != CommandInterface.ACK_SUCCESSFUL:
            self._logger.error(f'Unknown return status code in {commandString} command: {msg.payload[0]}')
            return False
        
        return True


    def _parseResponse(self, msg: Message):        
        if not self._checkStatusByte(msg):
            return

        # extract the raw command code from the acked command code
        commandCode = (msg.command_code & ~CommandInterface.ACK_BIT_MASK) & CommandInterface.BYTE_MASK

        if msg.payload is None or len(msg.payload) < 1:
            self._logger.error(f'Empty payload in message returned from driver; command code {msg.command_code}')
            return

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
        elif commandCode == CommandInterface.CMD_GET_MEAN_CURRENT:
            self._parseMeanCurrent(msg)
        elif commandCode == CommandInterface.CMD_GET_LOG_MESSAGE:
            self._parseLogMessage(msg)
        elif commandCode in CommandInterface.NO_RESPONSE_PAYLOAD_COMMANDS:
            # these commands do not return any data in the payload, only an
            # ack code; if we've made it to this point, then the status byte
            # check in _checkStatusByte was successful, so we know the command
            # completed successfully
            self._logger.info(f'Command {self._commandLabelMap[commandCode]} completed successfully')            
        else:
            self._logger.error('No handler set up to parse response for command code 0x{commandCode:x}')


    def requestVersion(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_VERSION
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseVersion(self, msg: Message):
        if len(msg.payload) != 4:
            self._logger.error(f'Invalid payload in version message (length = {len(msg.payload)})')
        
        self._logger.info(f'Motor driver version {msg.payload[1]}.{msg.payload[2]}.{msg.payload[3]}')
    
    
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
        if len(payload) != 9:
            self._logger.error(f'Invalid payload in status message (payload length {len(payload)})')
            return

        self._logger.debug('Successfully parsed motor status')
        self._driverStatus = MotorCommanderDataClasses.DriverStatus(msg.payload)


    def requestLogMessage(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_LOG_MESSAGE
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseLogMessage(self, msg: Message):
        payload = msg.payload
        if len(payload) < 3:
            self._logger.error('Invalid payload in log message')

        logMessagesRemaining = struct.unpack('H', payload[1:3])

        if logMessagesRemaining == CommandInterface.NO_LOG_MESSAGES_AVAILABLE:
            self._logger.warning('No log messages available')
            return
        elif logMessagesRemaining == 0:
            # we have retrieved the last message available
            self._driverStatus.IsLogMessageAvailable = False

        # logTimestamp = struct.unpack('I', payload[3:7])[0]
        messageLength = struct.unpack('H', payload[7:9])[0]
        # messageSeverity = payload[9]

        # check that payload length matches the expected message length
        if len(payload) != 10 + messageLength:
            self._logger.error('Received log message does not match expected length')

        logMessage = payload[10:].decode('ascii', 'ignore')

        # cut off the '\r\n' from the transmitted message
        self._logger.info(f'DRIVER LOG::{logMessage[:-2]}')        


    def rampMotorOn(self, driveLevel: int, rampTime_ms: int, forwardDirection: bool):
        if type(driveLevel) not in (int, float):
            raise TypeError('Drive level must be a number between 0 and 1000 (100%)')
        if driveLevel < 0 or driveLevel > 1000:
            raise ValueError('Drive level must be a number between 0 and 1000 (100%)')
        
        if type(rampTime_ms) != int:
            raise TypeError('Ramp time (ms) must be an integer of at least 25')
        if rampTime_ms < 25:
            raise ValueError('Ramp time (ms) must be an integer of at least 25')

        if type(forwardDirection) != bool:
            raise TypeError('Forward direction argument must be boolean')
        
        msg = Message()
        msg.command_code = CommandInterface.CMD_RAMP_MOTOR_ON

        msg.payload = struct.pack('H', int(driveLevel))
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
            self._logger.error(f'Invalid payload in motor position message (payload length {len(msg.payload)})')
            return
        
        self._logger.debug('Sucessfully parsed motor position')
        self._motorStatus.Position = struct.unpack('i', payload[1:5])[0]


    def requestMotorSpeed(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_MOTOR_SPEED
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseMotorSpeed(self, msg: Message):
        payload = msg.payload
        if len(payload) != 5:
            self._logger.error('Invalid payload in motor speed message')
            return

        self._logger.debug('Successfully parsed motor speed')
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
            self._logger.error('Invalid payload length in motor current message')
            return
        
        num_samples = struct.unpack('H', msg.payload[1:3])[0]
        if num_samples != CURRENT_DATA_MS*CURRENT_SENSOR_MEASUREMENTS_PER_MS:
            self._logger.error('Number of current samples returned does not match expected value')
            return

        unpackFormatString = f'{num_samples}f'
        currentMeasurementArray = struct.unpack(unpackFormatString, msg.payload[3:])
        self._motorStatus.Current = np.mean(currentMeasurementArray)
        self._logger.debug('Successfully parsed motor current')


    def requestMeanCurrent(self):
        msg = Message()
        msg.command_code = CommandInterface.CMD_GET_MEAN_CURRENT

        # motor current request payload contains number of ms of data to request
        # we will request 50 ms of data, which is our default tick time
        bytes = struct.pack('H', CURRENT_DATA_MS)
        msg.payload = bytes
        msg.set_crc()

        self._messageQueue.append(msg)


    def _parseMeanCurrent(self, msg: Message):
        payload = msg.payload

        # payload length = 1 ack byte + 4 byte floats mean current value
        payload_len = 5

        if len(payload) != payload_len:
            self._logger.error('Invalid payload length in mean current message')
            return
        
        self._motorStatus.Current = struct.unpack('f', payload[1:])[0]
        self._logger.debug('Successfully parsed mean current')


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


    def printStatus(self):
        driverState = MotorCommanderDataClasses.MotorControlState(self._driverStatus.ControllerState)
        
        self._logger.info('Motor driver status:')
        self._logger.info(f'\tState: {MotorCommanderDataClasses.MotorStatusLabel[driverState]}')
        self._logger.info(f'\tDirection: {"forward" if self._driverStatus.ForwardDirection else "reverse"}')
        self._logger.info(f'\tSleeping: {self._driverStatus.IsSleeping}')
        self._logger.info(f'\tFaulted: {self._driverStatus.IsFaulted}')
        self._logger.info(f'\tLog messages available? {self._driverStatus.IsLogMessageAvailable}')
        self._logger.info(f'\tMotor drive level: {self._driverStatus.DriveLevel}')
        self._logger.info(f'\tTime in state: {self._driverStatus.TimeInState_ms} ms')
        self._logger.info(f'\tMotor position: {self._motorStatus.Position}')
        self._logger.info(f'\tMotor speed: {self._motorStatus.Speed}')
        self._logger.info(f'\tMotor current: {self._motorStatus.Current}')

    
    def _buildCommandLabelMap(self):
        self._commandLabelMap[CommandInterface.ACK_SUCCESSFUL] = "command executed successfully"
        self._commandLabelMap[CommandInterface.ACK_INVALID_CRC] = "command failure; invalid CRC"
        self._commandLabelMap[CommandInterface.ACK_INVALID_COMMAND] = "command failure: invalid command code"
        self._commandLabelMap[CommandInterface.ACK_COMMAND_ERROR] = "command failure: error processing command"

        self._commandLabelMap[CommandInterface.CMD_ENABLE_MOTOR] = "enable motor"
        self._commandLabelMap[CommandInterface.CMD_DISABLE_MOTOR] = "disable motor"
        self._commandLabelMap[CommandInterface.CMD_SLEEP_DRIVER] = "sleep motor"
        self._commandLabelMap[CommandInterface.CMD_WAKE_DRIVER] = "wake motor"
        self._commandLabelMap[CommandInterface.CMD_CLEAR_FAULT] = "clear fault"

        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_POS] = "get motor position"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_PWM] = "get motor PWM"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_SPEED] = "get motor speed"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_DIR] = "get motor direction"
        self._commandLabelMap[CommandInterface.CMD_GET_MOTOR_CURRENT] = "get motor current"
        self._commandLabelMap[CommandInterface.CMD_GET_MEAN_CURRENT] = "get mean current"
        
        self._commandLabelMap[CommandInterface.CMD_RAMP_MOTOR_ON] = "ramp motor on"
        self._commandLabelMap[CommandInterface.CMD_RAMP_MOTOR_OFF] = "ramp motor off"
        self._commandLabelMap[CommandInterface.CMD_SET_MAX_DC] = "set maximum DC"

        self._commandLabelMap[CommandInterface.CMD_INJECT_FAULT] = "inject fault"
        self._commandLabelMap[CommandInterface.CMD_GET_VERSION] = "get driver version"
        self._commandLabelMap[CommandInterface.CMD_GET_STATUS] = "get driver status"
        self._commandLabelMap[CommandInterface.CMD_GET_LOG_MESSAGE] = "get log message"


    def getDriverStatus(self):
        return self._driverStatus


    def setStreamLogLevelInfo(self):
        self._streamHandler.setLevel(logging.INFO)
        self._logger.info('Setting stream log level to INFO')

    
    def setStreamLogLevelDebug(self):
        self._streamHandler.setLevel(logging.DEBUG)
        self._logger.info('Setting stream log level to DEBUG')

class MotorCommanderException(Exception):
    pass