import MotorCommander
import logging
import time
port = '/dev/ttyACM0'

# TO DO:
# - update sleep command to enforce 50 ms repeat rate
# - have an initialization section where we bring up the motor driver
# - create keyboard interface to send commands (enable/disable, sleep/wake, 
#   ramp on/off, get driver/motor status)

commander = MotorCommander.MotorCommander(port)
logger = logging.getLogger()

logger.info('Logging from top level script')

continueFlag = True

# main loop
while continueFlag:

    commander.tick()
    time.sleep(0.05)

