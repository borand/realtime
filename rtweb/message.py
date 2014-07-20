from __future__ import print_function

from common import get_host_ip
from simplejson import dumps, loads

class Message(object):
    """
    Class for defining validating and handling messages send between system components
    """

    def __init__(self, from_host=get_host_ip(), to='', msg=''):
        self.from_host = from_host
        self.to        = to
        self.msg       = msg

    def __str__(self):
        return "Message(FROM: %s, TO: %s, MSG: %s)" % (self.from_host, self.to, str(self.msg))

    def as_jsno(self):
        data = {"FROM" : self.from_host, "TO" : self.to, "MSG" : self.msg}
        return dumps(data)

    def decode(self, msg):
        data_dict = loads(msg)
        self.from_host = data_dict['FROM']
        self.to        = data_dict['TO']
        self.msg       = data_dict['MSG']
        return data_dict


def main():
    M = Message("192.168.1.10:/dev/ttyUSB0","ALL", "test")
    print(M)

    print(M.from_host)
    print(M.to)
    print(M.msg)

    msg = '{"TO": "you", "DATA": ["2014-06-13-21:58:53", 7, {"cmd": "adc", "data": [424, 391, 378, 378, 426]}], "FROM": "192.168.68.202:/dev/ttyUSB0"}'
    M.decode(msg)
    print(M.from_host)
    print(M.msg)
    print(M.to)

if __name__ == "__main__":
    main()