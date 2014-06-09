from __future__ import print_function
import tornado.httpserver
import tornado.web
import tornado.websocket
import tornado.ioloop
import tornado.gen

import tornadoredis
import simplejson
import os
import sh
import re
import logbook

from tornado.options import define, options
define("port", default=8888, help="run on the given port", type=int)

##########################################################################################
#
#
def get_host_ip():
    """
    parses ifconfig system command and returns host ip
    """
    ip_exp = re.compile(r'(?:eth\d.*?inet addr\:)(\d{1,3}\.\d{1,3}.\d{1,3}.\d{1,3})',re.DOTALL)
    ip_out = ip_exp.findall(sh.ifconfig().stdout)    
    if len(ip_out) > 0:        
        return  ip_out[0]
    else:
        return '127.0.0.1'

##########################################################################################
#
#
log = logbook.Logger('rtweb.py')
redis_host_ip = get_host_ip()
host_ip       = get_host_ip()
redis_pubsub_channel = 'rtweb'

c = tornadoredis.Client(host=redis_host_ip)
c.connect()


class MainHandler(tornado.web.RequestHandler):
    def get(self):
        # print(self.request)        
        self.render("rtweb.html", title="RT WEB", host_ip=host_ip, page_title='Test')

class CmdHandler(tornado.web.RequestHandler):
    def get(self):
        cmd  = self.get_argument("cmd", None)
        chan = self.get_argument("chan", None)
        #msg  = simplejson.dumps({'cmd' : cmd, 'chan' : chan, 'res' : 'OK'})
        #self.write('cmd= %s  para= %s' % (cmd, para))
        #print('CmdHandler(%s)' % cmd)
        #self.write(msg)
        c.publish(chan + '-cmd',cmd)

class BerHandler(tornado.web.RequestHandler):
    def get(self, ber1, ber2):        
        msg = '[%s, %s]' % (ber1, ber2)
        c.publish('rtweb',msg)
        self.write(msg)

class VoaHandler(tornado.web.RequestHandler):
    def get(self, voa):       
        
        msg  = '{"id" : "launch_power", "val" : %s}' % voa
        c.publish('rtweb',msg)
        self.write(msg)
      
        
class NewMessageHandler(tornado.web.RequestHandler):
    def post(self):
        message = self.get_argument('message')
        c.publish(redis_pubsub_channel, message)
        self.set_header('Content-Type', 'text/plain')
        self.write('sent: %s' % (message,))


class MessageHandler(tornado.websocket.WebSocketHandler):
    def __init__(self, *args, **kwargs):
        super(MessageHandler, self).__init__(*args, **kwargs)
        self.listen()

    @tornado.gen.engine
    def listen(self):
        self.client = tornadoredis.Client(redis_host_ip)
        self.client.connect()
        yield tornado.gen.Task(self.client.subscribe, redis_pubsub_channel)
        self.client.listen(self.on_message)

    def on_message(self, msg):        
        #log.debug(type(msg))
        if isinstance(msg,unicode):
            log.debug(msg)
        else:
            if msg.kind == 'message':
                #log.debug(str(simplejson.loads(msg.body)))
                self.write_message(str(msg.body))
            if msg.kind == 'disconnect':
                # Do not try to reconnect, just send a message back
                # to the client and close the client connection
                self.write_message('The connection terminated '
                                   'due to a Redis server error.')
                self.close()

    def on_close(self):
        if self.client.subscribed:
            self.client.unsubscribe(redis_pubsub_channel)
            self.client.disconnect()


application = tornado.web.Application([    
    (r'/', MainHandler),
    (r'/cmd/', CmdHandler),
    (r'/msg', NewMessageHandler),
    (r'/ber/(?P<ber1>0.\d+)/(?P<ber2>0.\d+)', BerHandler),
    (r'/voa/(?P<voa>\d+.\d+)', VoaHandler),
    (r'/websocket', MessageHandler),
    ],
    template_path=os.path.join(os.path.dirname(__file__), "templates"),
    static_path=os.path.join(os.path.dirname(__file__), "static"),
    debug=True,
    )

if __name__ == '__main__':
    tornado.options.parse_command_line()
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(8888)
    print('Demo is runing at %s:8888\nQuit the demo with CONTROL-C' % get_host_ip())
    tornado.ioloop.IOLoop.instance().start()