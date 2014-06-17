"""hardware.py - 

Simple module for communicating with ComPort firmware written for AVR328p.

Usage:
  hardware.py test [--dev=DEV | --test | --submit_to=SUBMIT_TO | --redishost=REDISHOST]
  hardware.py run [--dev=DEV | --test | --submit_to=SUBMIT_TO | --redishost=REDISHOST]
  hardware.py (-h | --help)

Options:
  -h, --help
  --dev=DEV              [default: /dev/ttyUSB0]
  --run=RUN              [default: True]
  --submit_to=SUBMIT_TO  [default: 192.168.1.10]
  --redishost=REDISHOST  [default: 192.168.1.10]

"""

import serial
import time
import re
import simplejson as sjson
import logbook
import redis

from threading import Thread,Event
from Queue import Queue
from datetime import datetime
from logbook import Logger
from docopt import docopt

# MY MODULES
from message import Message
from common import get_host_ip

##########################################################################################
# Global definitions
PARITY_NONE, PARITY_EVEN, PARITY_ODD = 'N', 'E', 'O'
STOPBITS_ONE, STOPBITS_TWO = (1, 2)
FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS = (5,6,7,8)
TIMEOUT = 2

EXCHANGE = 'ComPort'

##########################################################################################
#
class RedisSub(Thread):

    def __init__(self, interface, channel='cmd', host='127.0.0.1'):
        Thread.__init__(self)
        self.interface = interface
        self.signature = "{0:s}:{1:s}".format(get_host_ip(), self.interface.serial.port)
        self.redis     = redis.Redis(host=host)
        self.channel   = self.signature+"-cmd"
        self.pubsub    = self.redis.pubsub()
        self.Log       = Logger('RedisSub')
        self.Log.debug('__init__(channel=%s)' % self.channel)

        self.pubsub.subscribe(self.channel)
        self.start()
        self.setName('RedisSub-Thread')

    def __del__(self):        
        self.Log.info('__del__()')
        self.stop()

    def stop(self):
        self.Log.info('stop()')
        self.redis.publish(self.channel,'unsubscribe')
        time.sleep(1)        
        self.Log.info('stopped')

    def run(self):
        self.Log.debug('run()')
        try:
            for item in self.pubsub.listen():
                if item['data'] == "unsubscribe":
                    self.pubsub.unsubscribe()
                    self.Log.info("unsubscribed and finished")
                    break
                else:
                    cmd = item['data']
                    if isinstance(cmd,str):
                        self.Log.debug(cmd)
                        self.interface.send(item['data'])
                    else:
                        self.Log.debug(cmd)
        except Exception as E:
            error_msg = {'source' : 'RedisSub', 'function' : 'def run(self):', 'error' : E.message}
            self.redis.publish('error',sjson.dumps(error_msg))
                
        self.Log.debug('end of run()')

##########################################################################################
class ComPort(Thread):
    re_data        = re.compile(r'(?:<)(?P<cmd>\d+)(?:>)(.*)(?:<\/)(?P=cmd)(?:>)', re.DOTALL)
    re_next_cmd    = re.compile("(?:<)(\d+)(?:>\{\"cmd\":\")")

    redis_pub_channel = 'rtweb'
    
    def __init__(self,
                 port = '/dev/ttyUSB0',
                 packet_timeout=1,
                 baudrate=115200,       
                 bytesize=EIGHTBITS,    
                 parity=PARITY_NONE,    
                 stopbits=STOPBITS_ONE, 
                 xonxoff=0,             
                 rtscts=0,              
                 writeTimeout=None,     
                 dsrdtr=None,
                 host='127.0.0.1',
                 run=True):
        
        Thread.__init__(self)

        self.serial = serial.Serial(port, baudrate, bytesize, parity, stopbits, packet_timeout, xonxoff, rtscts, writeTimeout, dsrdtr)
        self.signature = "{0:s}:{1:s}".format(get_host_ip(), self.serial.port)
        self.redis_send_key = self.signature+'-send'
        self.redis_read_key = self.signature+'-read'
        self.log = Logger(self.signature)
        self.redis = redis.Redis(host=host)

        # TODO add checking for redis presence and connection
        if self.redis.ping():
            # Register the new instance with the redis exchange
            if not self.redis.sismember(EXCHANGE,self.signature):
                self.redis.sadd(EXCHANGE,self.signature)
        else:
            pass

        self.running = Event()
        self.buffer  = ''
        if run:
            self.start_thread()

        self.log.debug('ComPort(is_alive=%d, serial_port_open=%d, redis_host=%s)' % (self.is_alive(), not self.serial.closed, host))

    def __del__(self):
        self.log.debug("About to delete the object")
        self.close()
        time.sleep(1)
        self.log.debug("Closing serial interface")
        self.serial.close()
        if self.serial.closed:
            self.log.error("The serial connection still appears to be open")
        else:
            self.log.debug("The serial connection is closed")
        self.log.debug("Object deleted")
        if self.redis.sismember('ComPort',self.serial.port):
            self.redis.srem('ComPort',self.serial.port)
        
    def start_thread(self):
        '''
        Open the serial serial bus to be read. This starts the listening
        thread.
        '''

        self.log.debug('start_thread()')
        self.serial.flushInput()
        self.running.set()
        self.start()
        
    def open(self):
        if not self.serial.isOpen():
            self.serial.open()
        return self.serial.isOpen()
    
    def send(self, data, CR=True):
        '''Send command to the serial port
        '''
        if len(data) == 0:               
            return
        self.log.debug("send(cmd=%s)" % data)
        # Automatically append \n by default, but allow the user to send raw characters as well
        if CR:
            if (data[-1] == "\n"):
                pass            
            else:
                data += "\n"
            
        if self.open():
            try:
                self.serial.write(data)
                serial_error = 0
            except:
                serial_error = 1
        else:
            serial_error = 2
        self.redis.set(self.redis_send_key,data)
        return serial_error
    
    def read(self, waitfor=''):
        '''
        reads the data by waiting until new comport is found in the buffer and result can be read from the redis server
        '''

        serial_data = ''
        done = False
        to = time.clock()
        while time.clock() - to < TIMEOUT and not done:
            serial_data = self.redis.get(self.redis_read_key)
            done = waitfor in self.buffer and isinstance(serial_data,str)

        if not done:
            self.log.debug("read() did not find waitfor {:s} in self.buffer".format(waitfor))

        self.redis.delete(self.redis_read_key)
        return [done, serial_data]

    def query(self,cmd, **kwargs):
        """
        sends cmd to the controller and waits until waitfor is found in the buffer.
        """
        
        waitfor = kwargs.get('waitfor','')
        tag     = kwargs.get('tag','')
        json    = kwargs.get('json',1)
        delay   = kwargs.get('delay',0.01)

        if len(waitfor) < 1:
            next_cmd_num = self.re_next_cmd.findall(self.buffer)
            if len(next_cmd_num) > 0:
                waitfor = '<{:d}>{:s}"cmd":"'.format(int(next_cmd_num[0])+1,"{")

        self.log.debug('query(cmd=%s, waitfor=%s, tag=%s,json=%d, delay=%d):' % \
            (cmd, waitfor, tag, json, delay))

        self.send(cmd)
        time.sleep(delay)        
        query_data = self.read(waitfor=waitfor)
        if query_data[0]:
            try:
                query_data[1] = sjson.loads(query_data[1])
            except:
                query_data[0] = False
        return query_data

    def close(self):
        '''
        Close the listening thread.
        '''
        self.log.debug('close() - closing the worker thread')
        self.running.clear()

    def run(self):
        '''
        Run is the function that runs in the new thread and is called by
        start(), inherited from the Thread class
        '''
        
        try:            
            self.log.debug('Starting the listner thread')
            Msg = Message(self.signature)

            while(self.running.isSet()):
                bytes_in_waiting = self.serial.inWaiting()                
                
                if bytes_in_waiting:
                    new_data = self.serial.read(bytes_in_waiting)
                    self.buffer = self.buffer + new_data
                    #self.log.debug('found %d bytes inWaiting' % bytes_in_waiting)

                crlf_index = self.buffer.find('\r\n')

                if crlf_index > -1:
                    # self.log.debug('read line: ' + line)
                    line = self.buffer[0:crlf_index]
                    temp = self.re_data.findall(line)
                                       
                    if len(temp):
                        final_data = dict()
                        timestamp = datetime.now().strftime('%Y-%m-%d-%H:%M:%S')
                        final_data['timestamp'] = timestamp
                        final_data['raw']       = line
                        try:
                            final_data.update({'cmd_number' : sjson.loads(temp[0][0])})
                            final_data.update(sjson.loads(temp[0][1]))

                        except Exception as E:
                            final_data.update({'cmd_number' : -1})
                            error_msg = {'timestamp' : timestamp, 'from': self.signature, 'source' : 'ComPort', 'function' : 'def run() - inner', 'error' : E.message}
                            Msg.msg = error_msg
                            self.redis.publish('error',error_msg)

                        Msg.msg = final_data
                        self.redis.publish(self.redis_pub_channel, Msg.as_jsno())
                        self.redis.set(self.redis_read_key,Msg.as_jsno())

                    self.buffer = self.buffer[crlf_index+2:]

        except Exception as E:
            error_msg = {'source' : 'ComPort', 'function' : 'def run() - outter', 'error' : E.message}
            self.redis.publish('error',sjson.dumps(error_msg))
            self.log.error("Exception occured, within the run function: %s" % E.message)
        
        self.log.debug('Exiting run() function')

############################################################################################

if __name__ == '__main__':
    arguments = docopt(__doc__, version='Naval Fate 2.0')
    print("===============================================")
    print(arguments)

    dev = arguments['--dev']

    print("===============================================")
    print(dev)


    test_json = arguments['--test']
    run_main  = arguments['run']
    
    C = ComPort(dev)
    C.log.level = logbook.DEBUG
    
    if test_json:
        cmd_vector = ['idn', 'adc', 'dio', 'getwh', 'resetwh', 'peek 22', 'owrom', 'owsave 1','owload', \
                  'owsp','owdata','owwp 3 1', 'owrp 3', 'adsf']
        for cmd in cmd_vector:
            try:
                out = C.query(cmd)
                print out
            except Exception as E:
                print E    
    
    if run_main:
        R = RedisSub(C)
        try:
            while True:
                pass
        except KeyboardInterrupt:
            pass
        R.stop()

    C.close()

    print "All done"
