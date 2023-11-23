import serial
import time

sleepTime = 0.004
s = serial.Serial('/dev/ttyACM0', baudrate=921600)
s.read(s.in_waiting)

def readAndPrint():
    if (s.in_waiting > 0):
        b = s.read(s.in_waiting)
        t = b.decode('ascii', 'ignore')
        print(f'{len(b)}: {b}')

# outBytes = [i for i in range(0x40, 0x60)]
# outBytes.extend(outBytes)

outBytes = bytearray('RUSTqbcSTOP'.encode('utf8'))

b = s.write(outBytes)

while True:
    '''
    time.sleep(sleepTime)
    s.write(bytearray([0x52]))
    time.sleep(sleepTime)
    s.write(bytearray([0x53]))
    time.sleep(sleepTime)
    s.write(bytearray([0x54]))
    time.sleep(sleepTime)
    readAndPrint()
    '''
    time.sleep(0.1)

    readAndPrint()
