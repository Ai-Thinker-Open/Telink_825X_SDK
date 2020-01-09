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

__version__ = "0.2 dev"

PYTHON2 = sys.version_info[0] < 3  # True if on pre-Python 3

CMD_GET_VERSION = 0x00
CMD_WRITE_FLASH = 0x01
CMD_READ_FLASH  = 0x02
CMD_ERASE_FLASH = 0x03

RES_WRITE_FLASH = 'OK_01'
RES_READ_FLASH  = 'OK_02'
RES_ERASE_FLASH = 'OK_03'

def uart_read(_port):
    data = ''
    while _port.inWaiting() > 0:
        data += _port.read_all().decode(encoding='utf-8')
    return str(data)

def uart_write(_port, data):
    _port.flushInput()
    _port.flushOutput()

    _port.write(data)

def wait_result(_port, res):
    wait_c = 0
    result = ''
    while True:
        result += uart_read(_port)
        if(len(result) > 5): 
            break
        time.sleep(0.005)
        wait_c += 1
        if(wait_c > 400): break

    if result.find(res) == -1:
        return False
    return True

def telink_flash_write(_port, addr, data):
    cmd_len = len(data) + 5
    if(addr < 0x4000): addr += 0x2C000
    uart_write(_port, struct.pack('>BHIB', CMD_WRITE_FLASH, cmd_len, addr, 0) + data)
    return wait_result(_port, RES_WRITE_FLASH)

def telink_flash_read(_port, addr, len_b):
    uart_write(_port, struct.pack('>BHIB', CMD_READ_FLASH, 5, addr,len_b))
    time.sleep(0.01)

    data = bytes()
    while True :
        if _port.inWaiting() > 0: data += _port.read_all()
        if len(data) > len_b + 5: break
        time.sleep(0.01)

    result = str(data[len_b:])
    if  result.find(RES_READ_FLASH) == -1: return False , 0
    else : return True , data[:len_b]

def telink_flash_erase(_port, addr, len_t):
    uart_write(_port, struct.pack('>BHIB', CMD_ERASE_FLASH, 5, addr, len_t))
    time.sleep(len_t * 0.03) #wait erase complect
    return wait_result(_port, RES_ERASE_FLASH)

def connect_chip(_port):

    _port.setRTS(True)
    _port.setDTR(True)

    time.sleep(0.1)

    _port.setRTS(False)
    time.sleep(0.15)
    _port.setDTR(False)

    uart_write(_port, struct.pack('>BH', CMD_GET_VERSION, 0))

    if wait_result(_port, "V"):
        return True
    return False

def erase_flash(_port, args):

    flash_addr = int(args.addr, 0)
    sector_len = int(args.len,  0)

    print("Erase Flash at " + args.addr + " " + args.len + " Sector ... ... ", end="")
    sys.stdout.flush()

    if telink_flash_erase(_port,flash_addr, sector_len):
        print("\033[3;32mOK!\033[0m")
    else:
        print("\033[3;31mFail!\033[0m")

def read_flash(_port, args):
    flash_addr = int(args.addr, 0)
    bytes_len  = int(args.len,  0)

    if bytes_len> 255:
        print("\033[3;31mThe MAX read len is 255 bytes!\033[0m")
        return

    print("Read Flash from " + args.addr + " " + args.len + " Bytes ... ... ", end="")
    sys.stdout.flush()

    data_c = 0
    res , data = telink_flash_read(_port,flash_addr, bytes_len)
    if res:
        print("\033[3;32mOK!\033[0m")
        for b in data:
            data_c += 1
            if data_c == 16: 
                print("%02x " %b)
                data_c = 0
            else :print("%02x " %b, end='')
        print('')
    else:
        print("\033[3;31mFail!\033[0m")

def burn(_port, args):

    print("Erase Flash at 0x4000 len 176 KB ... ... ", end="")
    sys.stdout.flush()

    if not telink_flash_erase(_port, 0x4000, 44):
        print("\033[3;31mFail!\033[0m")
        return
    
    print("\033[3;32mOK!\033[0m\r\nBurn Firmware :"  + args.filename)

    fo = open(args.filename, "rb")
    firmware_addr = 0
    firmware_size = os.path.getsize(args.filename)
    bar_len = 50

    while True:
        data = fo.read(256)
        if len(data) < 1: break

        if not telink_flash_write(_port, firmware_addr, data):
            print("\033[3;31mBurn firmware Fail!\033[0m")
            break

        firmware_addr += len(data)

        percent = (int)(firmware_addr *100 / firmware_size)
        sys.stdout.write("\r" + str(percent) + "% [\033[3;32m{0}\033[0m{1}]".format(">"*(int)((percent/100)*bar_len),"="*(bar_len-(int)((percent/100)*bar_len))))
        sys.stdout.flush()

    print("")
    fo.close()
    _port.close()

def burn_triad(_port, args):

    data = struct.pack('<I', int(args.productID)) + bytearray.fromhex(args.MAC) + bytearray.fromhex(args.Secret)
    if(len(data) != 26):
        print("\033[3;31mTriad Error!\033[0m")
        return

    print("Erase Flash at 0x78000 len 4 KB ... ... ",end="")
    sys.stdout.flush()

    if not telink_flash_erase(_port, 0x78000, 1):
        print("\033[3;31mFail!\033[0m")
        return
    print("\033[3;32mOK!\033[0m")

    print("Burn Triad to 0x78000 ... ... ",end="")
    sys.stdout.flush()

    if not telink_flash_write(_port, 0x78000, data):
        print("\033[3;31mFail!\033[0m")
        return
    print("\033[3;32mOK!\033[0m")

def main(custom_commandline=None):

    parser = argparse.ArgumentParser(description='Telink_Tools.py v%s - Telink BLE Chip Bootloader Utility' % __version__)

    parser.add_argument('--port','-p', help='Serial port device', default='ttyUSB0')

    subparsers = parser.add_subparsers(dest='operation', help='Run Telink_Tools.py -h for additional help')
    
    burn = subparsers.add_parser('burn', help='Download an image to Flash')
    burn.add_argument('filename', help='Firmware image')

    burn = subparsers.add_parser('burn_triad', help='Burn tmall triad')
    burn.add_argument('productID', help='productID')
    burn.add_argument('MAC', help='MAC')
    burn.add_argument('Secret', help='Secret')


    write_flash = subparsers.add_parser('write_flash', help='Write data to flash')
    write_flash.add_argument('addr', help='write addr')
    write_flash.add_argument('data', help='data to write')

    write_flash = subparsers.add_parser('read_flash', help='Read data from flash')
    write_flash.add_argument('addr', help='read addr')
    write_flash.add_argument('len',  help='len to read')

    erase_flash = subparsers.add_parser('erase_flash', help='erase 4K (a page)')
    erase_flash.add_argument('addr', help='erase start addr')
    erase_flash.add_argument('len',  help='number of sector to erase')

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

    print("Open " + args.port + " ... ... ", end="")
    
    _port = serial.Serial(args.port, 921600, timeout=0.5)

    if not _port.isOpen():
        _port.open()

    print('\033[3;32mSuccess!\033[0m\r\nConnect Board ... ... ', end="")

    if connect_chip(_port):
        print("\033[3;32mSuccess!\033[0m")
        operation_func(_port,args)
    else:
        print("\033[3;31mFail!\033[0m")

def _main():
    #try:
    main()
    # except FatalError as e:
    #     print('\nA fatal error occurred: %s' % e)
    #     sys.exit(2)


if __name__ == '__main__':
    _main()