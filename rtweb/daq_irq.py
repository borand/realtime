"""daq_irq.py -

module used to listen for new data on the redis channel and submit the data to sensoredweb

Usage:
  daq_irq.py test [--dev=DEV] [--submit_to=SUBMIT_TO] [--redishost=REDISHOST]
  daq_irq.py run  [--dev=DEV] [--submit_to=SUBMIT_TO] [--redishost=REDISHOST]
  daq_irq.py (-h | --help)

Options:
  -h, --help
  --dev=DEV              [default: /dev/ttyUSB0]
  --submit_to=SUBMIT_TO  [default: 192.168.1.10]
  --redishost=REDISHOST  [default: 192.168.1.10]

"""

import threading
from redis import Redis
from rq import Queue
from logbook import Logger
from datastore import submit
from docopt import docopt
import simplejson as sjson
import datetime
import time

from common import get_host_ip

##########################################################################################
# Define special processing functions for various sensor data
def process_hydro_power_data(data):
    print("process_hydro_power_data({})".format(data))
    return round(3600.0/((pow(2,16)*float(data[1]) + float(data[2]))/16.0e6*1024.0))

def process_hydro_wh_data(data):
    print("process_hydro_wh_data({})".format(data))
    return data[2]

def process_default(data):
    print("process_default({})".format(data))
    return data[1]

ProcessingFunctions = {'hydro_power' : process_hydro_power_data,\
                       'hydro_wh'    : process_hydro_wh_data,\
                       }

##########################################################################################
class IrqSubmit(threading.Thread):

    def __init__(self, channel='irq', host='192.168.1.10', submit_to='192.168.1.10'):
        threading.Thread.__init__(self)        
        self.redis     = Redis(host=host)
        self.submit_to = submit_to
        self.msg_count = 0
        self.busy = 0;
        self.last = []        
        self.Q = Queue(connection=Redis())
        self.last_q = self.Q.enqueue(submit,([[0,'0',0]]))
        self.channel   = channel

        self.pubsub    = self.redis.pubsub()
        self.Log       = Logger('IrqSubmit')
        
        self.pubsub.subscribe(self.channel)
        self.start()
        self.setName('IrqSubmit-Thread')

    def __del__(self):        
        self.Log.info('__del__()')
        self.stop()

    def stop(self):
        self.Log.info('stop()')
        self.busy = False
        self.redis.publish(self.channel,'KILL')
        time.sleep(1)        
        self.Log.info('  stopped')

    def run(self):
        self.Log.debug('run()')
        
        for item in self.pubsub.listen():
            if item['data'] == "KILL":
                self.pubsub.unsubscribe()
                self.Log.info("unsubscribed and finished")
                break
            if item['data'] == "ERROR_TEST":
                self.redis.publish('error', __name__ + ": ERROR_TEST")
            else:
                if item['type'] == 'message':
                    self.print_message(item)
                #self.process_message(item)

        self.Log.debug('end of run()')

    def print_message(self, item):
        try:

            msg         = sjson.loads(item['data'])
            device_data = msg['MSG']['data']
            timestamp   = msg['MSG']['timestamp']

            for data in device_data:
                sn     = data[0]
                processing_function = ProcessingFunctions.get(sn,process_default)
                val    = processing_function(data)
                print("Final dataset : sn = {0}, val = {1}".format(sn, val))

                threshold   = 0
                submit_data = [[sn, val]]
                self.last_enqueue = self.Q.enqueue(submit, submit_data,\
                                    timestamp=timestamp,\
                                    submit_to=self.submit_to,\
                                    threshold=threshold)

        except Exception as E:
            self.Log.error("print_message(): " + E.message)
            self.Log.error(item)

    def process_message(self, item):
        try:
            if item['type'] == 'message':
                msg = sjson.loads(item['data'])
                self.last = msg
                self.Log.debug('    msg=%s' % str(msg))
                timestamp = datetime.datetime.strptime(msg[0].split('.')[0],"%Y-%m-%d-%H:%M:%S")

                cmd = msg['MSG']['cmd']
                data = msg[2]['data']
                submit_data = None
                threshold   = 0
                self.Log.debug('process_message(type=%s, cmd=%s)' % (item['type'], cmd))
                
                if cmd == 'irq_0':                    
                    val         = round(3600.0/((pow(2,16)*data[2] + data[3])/16e6*1024))
                    submit_data = [[0,'power_W',val]]
                if cmd == 'irq_cWH':
                    val         = data[0]
                    submit_data = [[0,'power_cWh',val]]
                if cmd == 'irq_port_d':
                    val = data[0]
                    submit_data = []
                    threshold   = 1
                    for bit in range(4):
                        bit_val = val >> bit & 1
                        submit_data.append([0,'irq_port_d_%d' % bit, bit_val])

                if submit_data is not None:
                    self.last_enqueue = self.Q.enqueue(submit, submit_data,\
                                        timestamp=timestamp,\
                                        submit_to=self.submit_to,\
                                        threshold=threshold)
                
        except Exception as E:
            self.Log.error(E.message)
            self.redis.publish('error', E.message)

def StartIqrSubmit(channel, host, submit_to):
    print"StartIqrSubmit(%s, %s, %s)" % (channel, host, submit_to)
    try:
        I = IrqSubmit(channel=channel, host=host, submit_to=submit_to);
        print("===============================================")
        while True:
            pass
    except KeyboardInterrupt:
        pass
    I.stop()

    print "Exiting " + __name__

##########################################################################################
if __name__ == "__main__":
    arguments = docopt(__doc__, version='Naval Fate 2.0')
    print("===============================================")
    print(arguments)

    dev = arguments['--dev']

    if arguments['run']:
        channel   = 'rtweb'
        host      = get_host_ip()
        submit_to = arguments.get('--submit_to', get_host_ip())
        StartIqrSubmit(channel, host, submit_to)
