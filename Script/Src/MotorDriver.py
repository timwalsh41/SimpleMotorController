import serial
import os
import time
import logging

class MotorDriver:
    def __init__(self, port = None):
        self._logger = logging.getLogger(__name__)
        self._port = None

        if port is not None:
            self.setPort(port)
        else:
            self._logger.info('No port defined, please set port before connecting')

        self._serial = None


    def setPort(self, port):
        if os.path.exists(port):
            self._port = port
            self._logger.info(f'Set port to {port}')
        else:
            self._port = None
            self._logger.error(f'Port {port} not found')
            raise ValueError(f'Error setting port {port}: not found')


    def openPort(self):
        if self._serial is not None:
            self._logger.info(f'Port {self._port} already open')
        elif self._port is not None:
            # open serial port at 921600 with default 8N1 settings
            self._serial = serial.Serial(self._port, baudrate=921600)
            self._serial.flush()

            if self._serial.is_open:
                self._logger.info(f'Opened port {self._port}')
            else:
                self._logger.error(f'Error opening port {self._port}')
                self._serial = None
                raise ValueError(f'Unable to open port {self._port}')
        else:
            self._logger.error('Cannot open driver, must specify port first')
        

    def closePort(self):
        if self._serial is not None:
            if type(self._serial) is not str:
                self._serial.close()
                self._serial = None
                self._logger.info(f'Closed connection to motor driver on port {self._port}')


    def isConnected(self):
        return self._serial != None
        
    
    def sendBytes(self, msg: bytearray):
        if type(msg) != bytearray:
            raise ValueError('msg  must be a bytearray')
        
        self._serial.write(msg)
        self._serial.flush()
        
        # sleep to allow time for response
        time.sleep(0.002)


    def readBytes(self):
        # check bytes available to read, waiting for it to stabilize in case
        # we are in the middle of a transmission
        bytesAvailable = -1
        while bytesAvailable != self._serial.in_waiting:
            bytesAvailable = self._serial.in_waiting
            time.sleep(0.001)

        if bytesAvailable == 0:
            return bytearray()
        
        return self._serial.read(bytesAvailable)
