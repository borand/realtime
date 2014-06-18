"""
Module intended for testing all communication with the hardware

Usage:
  hardware.py [--dev=DEV]
  hardware.py query [--dev=DEV]
  hardware.py getsn [--dev=DEV]
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

def main(C):
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

def getsn(C):

    cmd_set = ['setsn s 0 arduino','setsn l 0 office',\
               'setsn a 0 A0','setsn a 1 A1','setsn a 2 A2','setsn a 3 A3','setsn a 4 A4','setsn a 5 A5',\
               'setsn b 0 B0','setsn b 1 B1','setsn b 2 B2','setsn b 3 B3','setsn b 4 B4','setsn b 5 B5',\
               'setsn d 0 D0','setsn d 1 D1','setsn d 2 D2','setsn d 3 D3','setsn d 4 D4','setsn d 5 D5',\
               'setsn i 0 i0','setsn i 1 i1','setsn i 2 i2','setsn i 3 i3','setsn i 4 i4','setsn i 5 i5',\
               ]
    for cmd in cmd_set:

        cmd_number = -2
        out = C.query(cmd.lower())
        if out[0]:
            cmd_number = out[1]['MSG']['cmd_number']
            raw        = out[1]['MSG']['raw']
        else:
            raw = out[1]
        print("cmd[{:02d}] : {:>20s} : {:b} : {:s}".format(cmd_number, cmd, out[0],raw))
        sleep(0.05)

    print("===============================================")
    out = C.query('getsn')
    if out[0]:
        print("SN       :" + out[1]['MSG']['data']['SN'])
        print("location :" + out[1]['MSG']['data']['location'])
        print("PORTB    :" + str(out[1]['MSG']['data']['PORTB']))
        print("PORTD    :" + str(out[1]['MSG']['data']['PORTD']))
        print("ADC      :" + str(out[1]['MSG']['data']['ADC']))
        print("IRQ      :" + str(out[1]['MSG']['data']['IRQ']))
    else:
        print(str(out))

    out = C.query('adc')
    if out[0]:
        print(out[1]['MSG']['data'])


def submit_test(C):
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


def query(C):
    try:
        print("Initial buffer: " + C.buffer)
        print("===============================================")
        out = C.query('idn')
        print(str(out))
        print("===============================================")
        print("Final buffer: " + C.buffer)
    except Exception as E:
        print(E.message)

if __name__ == '__main__':
    arguments = docopt(__doc__, version='Naval Fate 2.0')
    print("===============================================")
    device = arguments['--dev']
    print(device)

    try:
        C = comport.ComPort(device)
        C.log.level = 10
        sleep(0.5)
        out = C.query('idn')

        if arguments['query']:
            query(C)
        if arguments['getsn']:
            getsn(C)
        elif arguments['submit']:
            submit_test(C)
        else:
            main(C)

    except:
        pass
    C.close()