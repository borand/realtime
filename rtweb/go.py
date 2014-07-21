__author__ = 'andrzej'

import time
from datetime import datetime, date

import psutil
from message import Message
from ablib.util.common import get_host_ip
from redis import Redis


PROCESS_OF_INTEREST = ['comport.py', 'rtweb.py', 'daq_irq.py', 'manage.py', '/venv/bin/rqworker']

def get_python_process_list():
    plist = []
    print "======================================================="
    print "= get_python_process_list()"
    print "="
    process_list = psutil.process_iter()
    for x in process_list:

        if len(x.cmdline) > 0 and 'venv/bin/python' in x.cmdline[0]:
            if any([p in x.cmdline[1] for p in PROCESS_OF_INTEREST]):
                print "= " + x.cmdline[0] + " : " + x.cmdline[1]
                plist.append(x)
    print "======================================================="
    return plist

def kill_stack():
    process_list = get_python_process_list()
    for p in process_list:
        p.kill()


def publish_measurement(data, redis_pub_channel='rtweb'):
    filename = 'template'
    signature = "{0:s}:{1:s}".format(get_host_ip(), filename)
    r = Redis()
    try:
        Msg = Message(signature)
        final_data = dict()
        timestamp = datetime.now().strftime('%Y-%m-%d-%H:%M:%S')
        final_data['timestamp'] = timestamp
        final_data['raw']       = ''
        final_data['data']      = data
        Msg.msg = final_data
        r.publish(redis_pub_channel, Msg.as_jsno())

    except Exception as E:
        error_msg = {'source' : 'ComPort', 'function' : 'def run() - outter', 'error' : E.message}
        #self.redis.publish('error',sjson.dumps(error_msg))
        #self.log.error("Exception occured, within the run function: %s" % E.message)


x = time.mktime(date.today().timetuple())

if __name__ == '__main__':
    get_python_process_list()
