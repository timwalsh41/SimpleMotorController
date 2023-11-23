import MotorCommander
import logging
import time
port = '/dev/ttyACM0'

# TO DO:
# - have an initialization section where we bring up the motor driver
# - create keyboard interface to send commands (enable/disable, sleep/wake, 
#   ramp on/off, get driver/motor status)
# - print status update regularly

PRINT_UPDATE_TIME_S = 5

loopTime = 1
printUpdateTimer = time.time()

commander = MotorCommander.MotorCommander(port)
logger = logging.getLogger('TestScript')

logger.info('Logging from top level script')

continueFlag = True

# main loop
while continueFlag:
    loopStart = time.time()

    commander.tick()
    
    loopRunTime = time.time() - loopStart
    sleepTime = loopTime - loopRunTime

    if (time.time() - printUpdateTimer) > PRINT_UPDATE_TIME_S:
        printUpdateTimer = time.time()
        commander.printStatus()

    if sleepTime > 0:
        time.sleep(sleepTime)
