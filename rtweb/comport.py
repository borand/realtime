"""hardware.py - 

Simple module for communicating with ComPort firmware written for AVR328p.

Usage:
  hardware.py test [--dev=DEV]
  hardware.py [--dev=DEV | --test]
  hardware.py (-h | --help)

Options:
  -h, --help
  --dev=DEV              [default: /dev/ttyUSB0]  
  --submit_to=SUBMIT_TO  [default: 192.168.1.133]

"""

import serial
import struct
import time
import re
import simplejson as sjson
import logbook

from threading import Thread,Event
from Queue import Queue, Empty
from warnings import *
from datetime import datetime
from logbook import Logger
from docopt import docopt

import redis

PARITY_NONE, PARITY_EVEN, PARITY_ODD = 'N', 'E', 'O'
STOPBITS_ONE, STOPBITS_TWO = (1, 2)
FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS = (5,6,7,8)
TIMEOUT = 2

log = Logger('ComPort')
log.info("2013.12.15 20:23")
log.level = logbook.ERROR

##########################################################################################
#
class RedisSub(Thread):

    def __init__(self, interface, channel='cmd', host='127.0.0.1'):
        Thread.__init__(self)
        self.interface = interface
        self.redis     = redis.Redis(host=host)
        self.channel   = channel      
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
        for item in self    .pubsub.listen():
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

                
        self.Log.debug('end of run()')

##########################################################################################
class ComPort(Thread):    
    read_q         = Queue()    
    redis          = redis.Redis()
    re_data        = re.compile(r'(?:<)(?P<cmd>\d+)(?:>)(.*)(?:<\/)(?P=cmd)', re.DOTALL)
    redis_send_key = 'ComPort-send'
    redis_read_key = 'ComPort-read'
    redis_pub_channel = 'rtweb'
    redis_sub_channel = 'cmd'
    
    
    def __init__(self,
                 port = 8,
                 packet_timeout=2,
                 baudrate=115200,       
                 bytesize=EIGHTBITS,    
                 parity=PARITY_NONE,    
                 stopbits=STOPBITS_ONE, 
                 xonxoff=0,             
                 rtscts=0,              
                 writeTimeout=None,     
                 dsrdtr=None            
                 ):

        '''
        Initialise the asynchronous serial object
        '''
        
        Thread.__init__(self)
        self.serial = serial.Serial(port, baudrate, bytesize, parity, stopbits, packet_timeout, xonxoff, rtscts, writeTimeout, dsrdtr)
        
        self.running = Event()
        self.buffer  = ''
        self.log = Logger('ComPort')
        log.info('ComPort(is_alive=%d, serial_port_open=%d)' % (self.is_alive(), not self.serial.closed))

    def __del__(self):
        log.debug("About to delete the object")
        self.close()
        log.debug("Closing serial interface")
        self.serial.close()
        if self.serial.closed:
            log.error("The serial connection still appears to be open")
        else:
            log.debug("The serial connection is closed")
        log.debug("Object deleted")
        
    def start_thread(self):
        '''
        Open the serial serial bus to be read. This starts the listening
        thread.
        '''

        log.debug('start_thread()')
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
        log.debug("send(cmd=%s)" % data)
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
        read data in the serial port buffer
        - check if the serial port is open 
        - attempt to read all data availiable in the buffer
        - pack the serial data and the serial errors
        '''
       
        serial_data = ''
        if self.is_alive():
            try:
                return self.read_q.get(1,1)
            except:
                return ''

        if self.open():
            try:
                to = time.clock()                
                done = 0
                while time.clock() - to < TIMEOUT and not done:
                    if self.is_alive():
                        if not self.read_q.empty():
                            tmp = self.read_q.get_nowait()
                            serial_data += tmp[1]
                    else:                        
                        n = self.serial.inWaiting()
                        if n > 0:
                            serial_data += self.serial.read(n)
                    if waitfor in serial_data:
                        done = 1
                serial_error = 0
            except:
                serial_error = 1
        else:
            serial_error = 2
        
        self.redis.set(self.redis_read_key,serial_data)
        return (serial_error, serial_data)
    
    def query(self,cmd, **kwargs):
        """
        sends cmd to the controller and watis until waitfor is found in the buffer.
        """
        
        waitfor = kwargs.get('waitfor','\r\n')
        tag     = kwargs.get('tag','')
        json    = kwargs.get('json',0)
        delay   = kwargs.get('delay',0)

        self.log.debug('query(cmd=%s, waitfor=%s, tag=%s,json=%d, delay=%d):' % \
            (cmd, waitfor, tag, json, delay))

        query_data = ''
        self.send(cmd)
        time.sleep(delay)        
        out = self.read(waitfor)
        query_data = out      
        return query_data

    def close(self):
        '''
        Close the listening thread.
        '''
        log.debug('close() - closing the worker thread')
        self.running.clear()

    def run(self):
        '''
        Run is the function that runs in the new thread and is called by
        start(), inherited from the Thread class
        '''
        
        try:            
            log.debug('Starting the listner thread')            
            while(self.running.isSet()):
                bytes_in_waiting = self.serial.inWaiting()                
                
                if bytes_in_waiting:
                    new_data = self.serial.read(bytes_in_waiting)
                    self.buffer = self.buffer + new_data
                    #log.debug('found %d bytes inWaiting' % bytes_in_waiting)

                crlf_index = self.buffer.find('\r\n')
                if crlf_index > 0:
                    # log.debug('read line: ' + line)
                    timestamp = datetime.now().strftime('%Y-%m-%d-%H:%M:%S')
                    line = self.buffer[0:crlf_index]
                    temp = self.re_data.findall(line)
                                       
                    if len(temp):
                        try:
                            final_data = [timestamp, sjson.loads(temp[0][0]), sjson.loads(temp[0][1])]
                            self.redis.publish('irq',sjson.dumps(final_data))
                        except Exception as E:
                            log.error(E.message + "  line %s" % line)                            
                            final_data = [timestamp, -1, line] 
                        
                        # final_data = [timestamp, line]
                        self.redis.publish(self.redis_pub_channel,sjson.dumps({'id':'debug_console','data':final_data}))
                        self.read_q.put(final_data)
                    self.buffer = self.buffer[crlf_index+2:]                        

        except Exception as E:
            log.error("Exception occured, within the run function: %s" % E.message)
        
        log.debug('Exiting run() function')

############################################################################################
    def process_q(self):
        """
        """
        if not self.read_q.empty():
            q_data = self.read_q.get(1,1)
            log.debug('q_data = %s' % str(q_data[1]))            
        else    :
            log.debug('read_q is empty')

if __name__ == '__main__':
    #log.level = logbook.ERROR
    test_json = 0;
    run       = 1;

    cmd_vector = ['idn', 'adc', 'dio', 'getwh', 'resetwh', 'peek 22', 'owrom', 'owsave 1','owload', \
                  'owsp','owdata','owwp 3 1', 'owrp 3', 'adsf']
    C = ComPort('/dev/ttyUSB0')
    C.log.level = logbook.ERROR
    C.send('C')
    C.start_thread()
    
    if test_json:
        for cmd in cmd_vector:
            try:
                out = C.query(cmd)
                print out
            except Exception as E:
                print E    
    if run:
        R = RedisSub(C)
        while True:
            pass

    R.stop()
    C.close()

    print "All done"
