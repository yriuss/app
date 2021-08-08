import sys
import serial
from collections import Counter
from itertools import islice
import argparse
import numpy as np
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description='Read N ADC elements from serial')
parser.add_argument("-b", "--baudrate", default=115200,
                    help="Baud rate for the serial communication")
parser.add_argument("-d", "--device", default="/dev/ttyACM0",
                    help="Port name for the serial interface (default: /dev/ttyACM0)")
parser.add_argument("-n", default=200000, type=int, dest="size",
                    help="Number of 2-byte integers to read from serial")

args = parser.parse_args()

def g(s):
    while 1:
        d = s.read(2)
        yield (d[0] + 256*d[1]) & 0xFFFFC

with serial.Serial(args.device, args.baudrate) as ser:
    while ser.read(1)[0] < 16:
        pass
    ser.read(1)
    lst = list(islice(g(ser), 0, args.size))

plt.plot(lst, "+-")
plt.show()
