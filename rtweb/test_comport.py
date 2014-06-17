"""


"""
from __future__ import print_function

import comport
import simplejson as sj

__author__ = 'andrzej'


def main():
    C = comport.ComPort('/dev/ttyUSB1')
    cmd_set = ['idn','adc','peek 2b']
    for cmd in cmd_set:
        out = print(C.query(cmd))
        print(str(out))
        # if C.query(cmd)['MSG']['cmd_number'] > 0:
        #     print(cmd + ' pass')
        # else:
        #     print(cmd + ' fail')
    C.close()


if __name__ == '__main__':
    #arguments = docopt(__doc__, version='Naval Fate 2.0')
    print("===============================================")
    main()