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
    DriveLevel: float
    TimeInState_ms: int

    def __init__(self, payload: bytearray):
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

        self.TimeInState_ms = struct.unpack('I', payload[6:])[0]


@dataclass
class MotorStatus:
    Position: float
    Speed: float
    Current: float