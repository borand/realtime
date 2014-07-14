__author__ = 'andrzej'

import psutil

process_list = psutil.process_iter()

PROCESS_OF_INTEREST = ['comport.py', 'rtweb.py', 'daq_irq.py' 'rqworker']

def get_python_process_list():
    for x in process_list:
        if "python" in x.name:
            print x


if __name__ == '__main__':
    get_python_process_list()
