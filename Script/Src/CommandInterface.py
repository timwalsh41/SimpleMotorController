# This must be kept in sync with Core/Inc/CommandInterface.h
START_SEQUENCE = [0x52, 0x55, 0x53, 0x54];  # R,U,S,T
STOP_SEQUENCE = [0x53, 0x54, 0x4F, 0x50];   # S,T,O,P

# number of bytes in CRC
CRC_LENGTH = 2

# Minimum message length consists of start sequence, stop sequence,
# command code, and 2-byte CRC
MINIMUM_MESSAGE_LENGTH = len(START_SEQUENCE) + len(STOP_SEQUENCE) + 1 + CRC_LENGTH

# Response code for command processed successfully
ACK_SUCCESSFUL = 0x2B;      # '+'

# Response code for invalid CRC in original message
ACK_INVALID_CRC = 0x28;	    # '('

# Response code for invalid command code
ACK_INVALID_COMMAND = 0x29; # ')'

# Response code for command processing error
ACK_COMMAND_ERROR = 0x2A; 	# '*'

# bit mask for setting the ackowledge flag in a command code
ACK_BIT_MASK = 0x80;
BYTE_MASK = 0xFF;

# special code returned if there are no log messages available to retrieve
NO_LOG_MESSAGES_AVAILABLE = 0xFFFF

# Commands
CMD_ENABLE_MOTOR = 0x30	        # '0'
CMD_DISABLE_MOTOR = 0x31	    # '1'
CMD_SLEEP_DRIVER = 0x32 	    # '2'
CMD_WAKE_DRIVER = 0x33		    # '3'
CMD_CLEAR_FAULT = 0x34		    # '4'
CMD_ZERO_MOTOR_POSITION = 0x35  # '5'
CMD_ZERO_MOTOR_ROTATION = 0x36  # '6'
CMD_ZERO_MOTOR_CURRENT = 0x37   # '7'
CMD_GET_MOTOR_POSITION = 0x40	# '@'
CMD_GET_MOTOR_PWM = 0x41	    # 'A'
CMD_GET_MOTOR_SPEED = 0x42	    # 'B'
CMD_GET_MOTOR_DIR = 0x43	    # 'C'
CMD_GET_MOTOR_CURRENT = 0x44    # 'D'
CMD_GET_MEAN_CURRENT = 0x45     # 'E'
CMD_GET_MOTOR_ROTATION = 0x46   # 'F'
CMD_RAMP_MOTOR_ON = 0x50	    # 'P'
CMD_RAMP_MOTOR_OFF = 0x51	    # 'Q'
CMD_SET_MAX_DC = 0x52		    # 'R'
CMD_INJECT_FAULT = 0x60 	    # '`'
CMD_GET_VERSION = 0x70		    # 'p'
CMD_GET_STATUS = 0x71		    # 'q'
CMD_GET_LOG_MESSAGE = 0x72	    # 'r'


# These commands do not return a payload with data, we group
# them here for standardized processing
NO_RESPONSE_PAYLOAD_COMMANDS = [CMD_ENABLE_MOTOR,
                                CMD_DISABLE_MOTOR,
                                CMD_SLEEP_DRIVER,
                                CMD_WAKE_DRIVER,
                                CMD_CLEAR_FAULT,
                                CMD_RAMP_MOTOR_ON,
                                CMD_RAMP_MOTOR_OFF,
                                CMD_SET_MAX_DC,
                                CMD_INJECT_FAULT,
                                CMD_ZERO_MOTOR_POSITION,
                                CMD_ZERO_MOTOR_ROTATION,
                                CMD_ZERO_MOTOR_CURRENT
                                ]

LOG_SEVERITY_MAP = {0: 'DEBUG',
                    1: 'INFO',
                    2: 'WARNING',
                    3: 'ERROR',
                    4: 'FATAL'}