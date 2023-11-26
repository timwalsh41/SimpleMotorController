from dataclasses import dataclass
from enum import Enum
import struct
import CommandInterface

# defined in MotorControlState.h
class MotorControlState(Enum):
    UNSET = 0
    DISABLED = 1
    SLEEPING = 2
    FAULT = 3
    IDLE = 4
    RUNNING = 5
    RAMP = 6

MotorStatusLabel =  {
                        MotorControlState.UNSET:'UNSET',
                        MotorControlState.DISABLED: 'DISABLED',
                        MotorControlState.SLEEPING: 'SLEEPING',
                        MotorControlState.FAULT: 'FAULT',
                        MotorControlState.IDLE: 'IDLE',
                        MotorControlState.RUNNING: 'RUNNING',
                        MotorControlState.RAMP: 'RAMP'
                    }

@dataclass
class DriverStatus:
    ControllerState: MotorControlState
    ForwardDirection: bool
    IsFaulted: bool
    IsSleeping: bool
    IsLogMessageAvailable: bool
    DriveLevel: int
    TimeInState_ms: int

    def __init__(self, payload: bytearray = None):
        if payload is None:
            self.ControllerState = MotorControlState.UNSET
            self.ForwardDirection = True
            self.IsFaulted = False
            self.IsSleeping = False
            self.IsLogMessageAvailable = False
            self.DriveLevel = 0
            self.TimeInState_ms = 0
            return
        
        if type(payload) != bytearray or len(payload) != 9:
            raise ValueError('Invalid status message packet received')
        
        if payload[0] != CommandInterface.ACK_SUCCESSFUL:
            raise ValueError('Status message command was not successfully completed')

        self.ControllerState = payload[1]
        self.ForwardDirection = (payload[2] & 0x01) > 0
        self.IsFaulted = (payload[2] & 0x02) > 0
        self.IsSleeping = (payload[2] & 0x04) > 0
        self.IsLogMessageAvailable = (payload[2] & 0x08) > 0

        self.DriveLevel = struct.unpack('H', payload[3:5])[0]/10

        self.TimeInState_ms = struct.unpack('I', payload[5:])[0]


@dataclass
class MotorStatus:
    Position: float
    Speed: float
    Current: float

    def __init__(self, position = None, speed = None, current = None):
        if position is None:
            self.Position = 0
        else:
            try:
                self.Position = float(position)
            except:
                raise TypeError ('Position must be a numeric value')
            
        if speed is None:
            self.Speed = 0
        else:
            try:
                self.Speed = float(speed)
            except:
                raise TypeError ('Speed must be a numeric value')
            
        if current is None:
            self.Current = 0
        else:
            try:
                self.Current = float(current)
            except:
                raise TypeError ('Current must be a numeric value')