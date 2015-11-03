"""rtweb.py -

Main webserver for rtweb

Usage:
  rtweb.py [--port=PORT] [--host=HOST]

Options:
  --port=PORT  [default: 8888]

"""

from __future__ import print_function
import tornado.httpserver
import tornado.web
import tornado.websocket
import tornado.ioloop
import tornado.gen
import os


class MainHandler(tornado.web.RequestHandler):
    def get(self):
        self.render("test.html")

class Application(tornado.web.Application):
    def __init__(self):
        handlers = [
                (r'/', MainHandler),
                ]
        
        settings = dict(
            cookie_secret = "__TODO:_GENERATE_YOUR_OWN_RANDOM_VALUE_HERE__",
            template_path = os.path.join(os.path.dirname(__file__), "templates"),
            static_path   = os.path.join(os.path.dirname(__file__), "static"),
            debug         = True,
            xsrf_cookies  = False,
        )
        tornado.web.Application.__init__(self, handlers, **settings)

if __name__ == '__main__':

    template_path = os.path.join(os.path.dirname(__file__), "templates")
    static_path   = os.path.join(os.path.dirname(__file__), "static")

    app = Application()
    app.listen(8888)
    tornado.ioloop.IOLoop.instance().start()