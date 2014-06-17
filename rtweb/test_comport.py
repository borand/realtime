"""
Module intended for testing all communication with the hardware

Usage:
  hardware.py [--dev=DEV]
  hardware.py query [--dev=DEV]
  hardware.py submit [--dev=DEV]
  hardware.py (-h | --help)

Options:
  --dev=DEV              [default: /dev/ttyUSB0]
"""
from __future__ import print_function

import comport
from time import sleep
from docopt import docopt
from datastore import submit
from simplejson import loads
import simplejson as sj

__author__ = 'andrzej'

def main(device):
    C = comport.ComPort(device)
    sleep(1)
    C.query('idn')
    cmd_set = ['idn','peek 2b','test','adc','dio','getwh','resetwh','reset','owrom','owload','owsp','I','asdf',\
               'setsn s 0 arduino','setsn l 0 office','setsn a 0 A','setsn b 0 B','setsn d 0 D','setsn i 0 irq','getsn']

    C.log.level = 10
    for cmd in cmd_set:

        cmd_number = -2
        out = C.query(cmd)
        if out[0]:
            cmd_number = out[1]['MSG']['cmd_number']
            raw        = out[1]['MSG']['raw']
            raw        = ''
        else:
            raw = out[1]
        print("cmd[{:02d}] : {:>20s} : {:b} : {:s}".format(cmd_number, cmd, out[0],raw))
        sleep(0.05)

    C.close()

def submit_test(device):
    C = comport.ComPort(device)
    C.log.level = 10
    try:
        sleep(1)
        out = C.query('setsn a 0 0')
        out = C.query('adc')
        print("===============================================")
        if out[0]:
            msg = out[1]['MSG']
            if msg['cmd_number'] > -1:
                data = msg['data']
            else:
                data = 'none'
            print(data)
            status = submit(data, timestamp='now', submit_to='sensoredweb.herokuapp.com', port=80, threshold=0)
            print(status)

        print("===============================================")
    except Exception as E:
        print(E.message)
    C.close()

def query(device):
    C = comport.ComPort(device)
    C.log.level = 10
    sleep(0.5)
    try:
        out = C.query('idn')
        print("Initial buffer: " + C.buffer)
        print("===============================================")
        out = C.query('idn')
        print(str(out))
        print("===============================================")
        print("Final buffer: " + C.buffer)
    except Exception as E:
        print(E.message)
    C.close()

if __name__ == '__main__':
    arguments = docopt(__doc__, version='Naval Fate 2.0')
    print("===============================================")
    device = arguments['--dev']
    print(device)

    if arguments['query']:
        query(device)
    elif arguments['submit']:
        submit_test(device)
    else:
        main(device)