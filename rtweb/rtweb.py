"""rtweb.py -

Main webserver for rtweb

Usage:
  rtweb.py [--port=PORT] [--host=HOST]

Options:
  --port=PORT  [default: 8888]

"""

from __future__ import print_function
import uuid
import tornado.httpserver
import tornado.web
import tornado.websocket
import tornado.ioloop
import tornado.gen
import logging
import tornadoredis
import os
import logbook

from docopt import docopt
from ablib.util.common import get_host_ip

from redis import Redis

#define("port", default=8888, help="run on the given port", type=int)

##########################################################################################
#
#
log = logbook.Logger('rtweb.py')
redis_host_ip = get_host_ip()
host_ip       = get_host_ip()
redis_pubsub_channel = ('rtweb', 'error')

#c = tornadoredis.Client(host=redis_host_ip)
#c.connect()
R = Redis()

class MainHandler(tornado.web.RequestHandler):
    def get(self):
        interfaces = list(R.smembers('ComPort'))
        dynamic_content = { "title" : "RT WEB", "host_ip" : host_ip, "page_title" : 'Test', "interfaces" : interfaces}
        self.render("rtweb.html", title="RT WEB", host_ip=host_ip, page_title='Test', interfaces=interfaces)
        #self.render("rtweb.html", dynamic_content)

class ConsoleHandler(tornado.web.RequestHandler):
    def get(self):
        interfaces = list(R.smembers('ComPort'))
        # dynamic_content = { "title" : "RTConsole", "host_ip" : host_ip, "page_title" : 'Test', "interfaces" : interfaces}
        self.render("console.html", title="RTConsole", host_ip=host_ip, page_title='Test', interfaces=interfaces)
        #self.render("rtweb.html", dynamic_content)

class CmdHandler(tornado.web.RequestHandler):
    def get(self):
        cmd  = self.get_argument("cmd", None)
        chan = self.get_argument("chan", None)
        #msg  = simplejson.dumps({'cmd' : cmd, 'chan' : chan, 'res' : 'OK'})
        #self.write('cmd= %s  para= %s' % (cmd, para))
        #print('CmdHandler(%s)' % cmd)
        #self.write(msg)
        R.publish(chan + '-cmd',cmd)

class NewMessageHandler(tornado.web.RequestHandler):
    def post(self):
        message = self.get_argument('message')
        R.publish(redis_pubsub_channel, message)
        self.set_header('Content-Type', 'text/plain')
        self.write('sent: %s' % (message,))

class MessageHandler(tornado.websocket.WebSocketHandler):

    channel = 'comport'

    def __init__(self, *args, **kwargs):
        super(MessageHandler, self).__init__(*args, **kwargs)


    def open(self, chan):
        self.sub_channel = chan
        self.listen()

    @tornado.gen.engine
    def listen(self):
        self.client = tornadoredis.Client(redis_host_ip)
        self.client.connect()
        yield tornado.gen.Task(self.client.subscribe, self.sub_channel)
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

class SubscribeHandler(tornado.websocket.WebSocketHandler):
    def __init__(self, *args, **kwargs):
        super(SubscribeHandler, self).__init__(*args, **kwargs)
        print(self.get_argument("chan", 'default'))
        self.listen()

    @tornado.gen.engine
    def listen(self):
        #print(self.get_argument("chan", 'default'))
        self.client = tornadoredis.Client(redis_host_ip)
        self.client.connect()
        yield tornado.gen.Task(self.client.subscribe, ("subscribe"))
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

class TestHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("test.html", messages=TestWebSocketHandler.cache)

class TestWebSocketHandler(tornado.websocket.WebSocketHandler):
    """
    simple websocket server for bidirectional communication between client and server
    """

    # public means non authenticated
    # private means authenticated
    cache = []
    channels = {
        'public': {},
        'private': {}
    }

    def send_message(self, *args):
        """ alias to write_message """
        self.write_message(*args)

    def add_client(self, user_id=None):
        """
        Adds current instance to public or private channel.
        If user_id is specified it will be added to the private channel,
        If user_id is not specified it will be added to the public one instead.
        """
        if user_id is None:
            # generate a random uuid if it's an unauthenticated client
            self.channel = 'public'
            user_id = uuid.uuid1().hex
        else:
            self.channel = 'private'

        self.id = user_id
        self.channels[self.channel][self.id] = self
        print('Client connected to the %s channel.' % self.channel)

    def remove_client(self):
        """ removes a client """
        del self.channels[self.channel][self.id]

    @classmethod
    def broadcast(cls, message):
        """ broadcast message to all connected clients """
        clients = cls.get_clients()
        # loop over every client and send message
        for id, client in clients.iteritems():
            client.send_message(message)

    @classmethod
    def send_private_message(self, user_id, message):
        """
        Send a message to a specific client.
        Returns True if successful, False otherwise
        """
        try:
            client = self.channels['private'][str(user_id)]
        except KeyError:
            print('====debug====')
            print(self.channels['private'])
            print('client with id %s not found' % user_id)
            return False

        client.send_message(message)
        print('message sent to client #%s' % user_id)
        return True

    @classmethod
    def get_clients(self):
        """ return a merge of public and private clients """
        public = self.channels['public']
        private = self.channels['private']
        return dict(public.items() + private.items())

    def open(self):
        """ method which is called every time a new client connects """
        print('Connection opened.')

        # retrieve user_id if specified
        user_id = self.get_argument("user_id", None)
        # add client to list of connected clients
        self.add_client(user_id)
        # welcome message
        self.send_message("Welcome to nodeshot websocket server.")
        # new client connected message
        client_count = len(self.get_clients().keys())
        new_client_message = 'New client connected, now we have %d %s!' % (client_count, 'client' if client_count <= 1 else 'clients')
        # broadcast new client connected message to all connected clients
        self.broadcast(new_client_message)

        print(self.channels['private'])

    def on_message(self, message):
        """ method which is called every time the server gets a message from a client """
        if message == "help":
            self.send_message("Need help, huh?")
        print('Message received: \'%s\'' % message)

    def on_close(self):
        """ method which is called every time a client disconnects """
        print('Connection closed.')
        self.remove_client()

        client_count = len(self.get_clients().keys())
        new_client_message = '1 client disconnected, now we have %d %s!' % (client_count, 'client' if client_count <= 1 else 'clients')
        self.broadcast(new_client_message)

class ChatHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("test.html", messages=ChatSocketHandler.cache)

class ChatSocketHandler(tornado.websocket.WebSocketHandler):
    waiters = set()
    cache = []
    cache_size = 200

    def get_compression_options(self):
        # Non-None enables compression with default options.
        return {}

    def open(self, chan):
        print(chan)
        ChatSocketHandler.waiters.add(self)

    def on_close(self):
        ChatSocketHandler.waiters.remove(self)

    @classmethod
    def update_cache(cls, chat):
        cls.cache.append(chat)
        if len(cls.cache) > cls.cache_size:
            cls.cache = cls.cache[-cls.cache_size:]

    @classmethod
    def send_updates(cls, chat):
        logging.info("sending message to %d waiters", len(cls.waiters))
        for waiter in cls.waiters:
            try:
                waiter.write_message(chat)
            except:
                logging.error("Error sending message", exc_info=True)

    def on_message(self, message):
        logging.info("got message %r", message)
        parsed = tornado.escape.json_decode(message)
        chat = {
            "id": str(uuid.uuid4()),
            "body": parsed["body"],
            }
        print()
        chat["html"] = tornado.escape.to_basestring(
            self.render_string("message.html", message=chat))

        ChatSocketHandler.update_cache(chat)
        ChatSocketHandler.send_updates(chat)

class Application(tornado.web.Application):
    def __init__(self):
        handlers = [
                (r'/', MainHandler),
                (r'/console', ConsoleHandler),
                (r'/cmd/', CmdHandler),
                (r'/msg', NewMessageHandler),
                (r'/websocket/(?P<chan>.*)', MessageHandler),
                (r'/sub', SubscribeHandler),
                (r'/testsoc/(?P<user_id>.*)', TestWebSocketHandler),
                (r'/test', ChatHandler),
                (r"/chatsocket/(?P<chan>.*)", ChatSocketHandler),
                ]
        settings = dict(
            cookie_secret="__TODO:_GENERATE_YOUR_OWN_RANDOM_VALUE_HERE__",
            template_path=os.path.join(os.path.dirname(__file__), "templates"),
            static_path=os.path.join(os.path.dirname(__file__), "static"),
            debug=True,
            xsrf_cookies=True,
        )
        tornado.web.Application.__init__(self, handlers, **settings)

if __name__ == '__main__':
    print("=== RTWEB ===")
    arguments = docopt(__doc__, version='Naval Fate 2.0')
    print(arguments)
    port = int(arguments['--port'])
    app = Application()
    app.listen(port)
    print('RTWEB is running at %s:%d\nQuit the demo with CONTROL-C' % (get_host_ip(), port))
    tornado.ioloop.IOLoop.instance().start()