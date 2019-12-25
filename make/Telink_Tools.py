import argparse
import base64
import binascii
import copy
import hashlib
import inspect
import io
import os
import shlex
import struct
import sys
import time
import zlib
import string

try:
    import serial
except ImportError:
    print("Pyserial is not installed for %s. Check the README for installation instructions." % (sys.executable))
    raise

# check 'serial' is 'pyserial' and not 'serial' https://github.com/espressif/esptool/issues/269
try:
    if "serialization" in serial.__doc__ and "deserialization" in serial.__doc__:
        raise ImportError("""
Telink_Tools.py depends on pyserial, but there is a conflict with a currently installed package named 'serial'.
You may be able to work around this by 'pip uninstall serial; pip install pyserial' \
but this may break other installed Python software that depends on 'serial'.
There is no good fix for this right now, apart from configuring virtualenvs. \
See https://github.com/espressif/esptool/issues/269#issuecomment-385298196 for discussion of the underlying issue(s).""")
except TypeError:
    pass  # __doc__ returns None for pyserial

try:
    import serial.tools.list_ports as list_ports
except ImportError:
    print("The installed version (%s) of pyserial appears to be too old for Telink_Tools.py (Python interpreter %s). "
          "Check the README for installation instructions." % (sys.VERSION, sys.executable))
    raise

__version__ = "0.1 dev"

PYTHON2 = sys.version_info[0] < 3  # True if on pre-Python 3

def connect_chip(port):
    print("Connect board with " + port + "...")

    _port = serial.Serial(port, 115200)
    _port.setRTS(True)
    _port.setDTR(True)

    time.sleep(0.3)

    _port.setRTS(False)
    time.sleep(2)
    _port.setDTR(False)

    buf = b'\x00\x00\x03'
    

    _port.write(buf)
    data = serial .readline()
    print(data)


    _port.close()

def burn(args):
    print "burn fun"

def main(custom_commandline=None):

    parser = argparse.ArgumentParser(description='Telink_Tools.py v%s - Telink BLE Chip Bootloader Utility' % __version__)

    parser.add_argument('-p', help='Serial port device', default='ttyUSB0')

    subparsers = parser.add_subparsers(dest='operation', help='Run Telink_Tools.py -h for additional help')
    
    burn = subparsers.add_parser('burn', help='Download an image to Flash')
    burn.add_argument('filename', help='Firmware image')

    write_flash = subparsers.add_parser('write_flash', help='Write data to flash')
    write_flash.add_argument('addr', help='write addr')
    write_flash.add_argument('data', help='data to write')

    write_flash = subparsers.add_parser('read_flash', help='Read data from flash')
    write_flash.add_argument('addr', help='read addr')
    write_flash.add_argument('len',  help='len to read')

    erase_flash = subparsers.add_parser('erase_flash', help='erase 4K (a page)')
    erase_flash.add_argument('addr', help='read addr')

    args = parser.parse_args(custom_commandline)

    print('Telink_Tools.py v%s' % __version__)
    
    if args.operation is None:
        parser.print_help()
        sys.exit(1)

    operation_func = globals()[args.operation]

    if PYTHON2:
        # This function is depreciated in Python3
        operation_args = inspect.getargspec(operation_func).args
    else:
        operation_args = inspect.getfullargspec(operation_func).args

    connect_chip(args.p)

    #operation_func(args)

def _main():
    try:
        main()
    except FatalError as e:
        print('\nA fatal error occurred: %s' % e)
        sys.exit(2)


if __name__ == '__main__':
    _main()