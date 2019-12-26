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

def telink_read(_port):
    data = ''
    while _port.inWaiting() > 0:
        data += str(_port.read_all())
    return data

def telink_write(_port, data):
    _port.write(data)

def connect_chip(_port):

    _port.setRTS(True)
    _port.setDTR(True)

    time.sleep(0.3)

    _port.setRTS(False)
    time.sleep(2.3)
    _port.setDTR(False)

    data = telink_read(_port)

    if string.find(data,'boot loader ready') != -1:
        return True
    return False

def burn(_port, args):
    print "Burn firmware: " + args.filename

    cmd = 0x03
    cmd_len = 5
    flash_addr = 0x4000
    telink_write(_port, struct.pack('>BHIB', cmd, cmd_len, flash_addr, 48))

    print "Erase Flash at 0x4000 len 192 KB ... ..."

    time.sleep(2) #wait erase complect
    result = telink_read(_port)

    if string.find(result,'OK') == -1:
        print "Erase Flash Fail!"

    cmd = 0x1
    fo = open(args.filename, "rb")
    firmware_addr = 0
    firmware_size = os.path.getsize(args.filename)

    while True:
        data = fo.read(256)

        if len(data) < 1: break

        cmd_len = len(data) + 5
        flash_addr = firmware_addr

        if(flash_addr < 0x4000): flash_addr += 0x30000

        telink_write(_port, struct.pack('>BHIB', cmd, cmd_len, flash_addr,cmd) + data)
        firmware_addr += len(data)

        time.sleep(0.03)

        result = telink_read(_port)

        if string.find(result,'OK') == -1:
            print "Burn firmware Fail!"
            break

        percent = (firmware_addr *100 / firmware_size)
        sys.stdout.write("\r" + str(percent) + "% [{0}{1}]".format(">"*(percent/10),"="*(10-percent/10)))
        sys.stdout.flush()

    print ""
    fo.close()
    _port.close()
    
def main(custom_commandline=None):

    parser = argparse.ArgumentParser(description='Telink_Tools.py v%s - Telink BLE Chip Bootloader Utility' % __version__)

    parser.add_argument('--port','-p', help='Serial port device', default='ttyUSB0')

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

    # if PYTHON2:
    #     # This function is depreciated in Python3
    #     operation_args = inspect.getargspec(operation_func).args
    # else:
    #     operation_args = inspect.getfullargspec(operation_func).args

    print("Open " + args.port + " ... ...")
    
    _port = serial.Serial(args.port, 115200, timeout=0.5)

    if not _port.isOpen():
        _port.open()

    print('Success!')

    if connect_chip(_port):
        print("Connect Board Success ...")
        operation_func(_port,args)
    else:
        print("Connect Board Fail ...")

def _main():
    #try:
    main()
    # except FatalError as e:
    #     print('\nA fatal error occurred: %s' % e)
    #     sys.exit(2)


if __name__ == '__main__':
    _main()