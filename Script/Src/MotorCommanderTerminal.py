from MotorCommander import MotorCommander
import termios
import sys
import tty

def getch():
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

def read_single_keypress():
    """Read a single keypress from stdin and return the resulting character."""
    # Save the terminal settings
    old_settings = termios.tcgetattr(sys.stdin)
    try:
        # Set the terminal to raw mode
        tty.setraw(sys.stdin.fileno())
        # Read a single character
        ch = sys.stdin.read(1)
    finally:
        # Restore the terminal settingsc
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
    return ch

commander = MotorCommander(port='/dev/ttyACM0', daemonize=True, tickTime_s=1)

while True:
    # c = getch()
    c = read_single_keypress()

    if c == 'h':
         # print command menu
         print('Commands:')
         print('\th:\tprint this menu')
         print('\tp:\tprint status')
         print('\t0:\tramp motor to zero')
         print('\t1:\tramp motor to forward direction 10%')
         print('\t2:\tramp motor to forward direction 25%')
         print('\t3:\tramp motor to forward direction 50%')
         print('\t4:\tramp motor to forward direction 75%')
         print('\t5:\tramp motor to reverse direction 10%')
         print('\t6:\tramp motor to reverse direction 25%')
         print('\t7:\tramp motor to reverse direction 50%')
         print('\t8:\tramp motor to reverse direction 75%')
         print('\t,:\tdecrease drive level by 5%')
         print('\t.:\tincrease drive level by 5%')
         print('\te:\tenable motor')
         print('\td:\tdisable motor')
         print('\ts:\tsleep driver')
         print('\tw:\twake driver')
         print('\tj:\tinject fault')
         print('\tc:\tclear fault')
         print('\ti:\tset display level to severity INFO')
         print('\tg:\tset display level to severity DEBUG')
         print('\tq:\tquit')
    elif c == 'q':
         commander.stop()
         sys.exit(0)
    elif c == 'p':
         commander.printStatus()
    elif c == 'e':
         commander.enableMotor()
    elif c == 'd':
         commander.disableMotor()
    elif c == 's':
         commander.sleepMotor()
    elif c == 'w':
         commander.wakeMotor()
    elif c == 'j':
         commander.injectFault()
    elif c == 'c':
         commander.clearFault()
    elif c == 'i':
         commander.setStreamLogLevelInfo()
    elif c == 'g':
         commander.setStreamLogLevelDebug()
    elif c == '0':
         commander.rampMotorOff(100)
    elif c == '1':
         commander.rampMotorOn(100, 100, True)
    elif c == '2':
         commander.rampMotorOn(250, 100, True)
    elif c == '3':
         commander.rampMotorOn(500, 100, True)
    elif c == '4':
         commander.rampMotorOn(750, 100, True)
    elif c == '5':
         commander.rampMotorOn(100, 100, False)
    elif c == '6':
         commander.rampMotorOn(250, 100, False)
    elif c == '7':
         commander.rampMotorOn(500, 100, False)
    elif c == '8':
         commander.rampMotorOn(750, 100, False)
    elif c == ',':
         commander.rampMotorOn(max(10*commander._driverStatus.DriveLevel-50,0), 25, commander._driverStatus.ForwardDirection)
    elif c == '.':
         commander.rampMotorOn(max(10*commander._driverStatus.DriveLevel+50,0), 25, commander._driverStatus.ForwardDirection)
    else:
         print(f'Unknown command: {c}')
