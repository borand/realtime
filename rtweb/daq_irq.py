import threading
import click
from redis import Redis
from rq import Queue
from logbook import Logger
from datastore import submit
import simplejson as sjson
import datetime
import time

from common import get_host_ip

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
        
        if channel=='':
            self.channel   = str(interface)
        else:
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
                self.process_message(item)
        self.Log.debug('end of run()')

    def process_message(self, item):
        try:
            if item['type'] == 'message':
                msg = sjson.loads(item['data'])
                self.last = msg
                self.Log.debug('    msg=%s' % str(msg))
                timestamp = datetime.datetime.strptime(msg[0].split('.')[0],"%Y-%m-%d-%H:%M:%S")

                cmd  = msg[2]['cmd']
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

@click.command()
@click.option('--channel', default='irq')
@click.option('--host', default=get_host_ip())
@click.option('--submit_to', default=get_host_ip())

def StartIqrSubmit(channel, host, submit_to):
    print"StartIqrSubmit(%s, %s, %s)" % (channel, host, submit_to)
    try:
        I = IrqSubmit(channel=channel, host=host, submit_to=submit_to);
        while True:
            pass
    except KeyboardInterrupt:
        pass
    I.stop()

    print "Exiting " + __name__

##########################################################################################
if __name__ == "__main__":
    StartIqrSubmit()