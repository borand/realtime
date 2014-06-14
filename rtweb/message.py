from __future__ import print_function

from simplejson import dumps, loads

class Message(object):
    """
    Class for defining validating and handling messages send between system components
    """

    def __init__(self, from_host='', to='', data=''):
        self.from_host = from_host
        self.to        = to
        self.data      = data

    def __str__(self):
        return "Message(FROM: %s, TO: %s, DATA: %s)" % (self.from_host, self.to, str(self.data))

    def as_jsno(self):
        data = {"FROM" : self.from_host, "TO" : self.to, "DATA" : self.data}
        return dumps(data)

    def decode(self, msg):
        data_dict = loads(msg)
        self.from_host = data_dict['FROM']
        self.to        = data_dict['TO']
        self.data      = data_dict['DATA']
        return data_dict


def main():
    M = Message("192.168.1.10:/dev/ttyUSB0","ALL", "test")
    print(M)

    print(M.from_host)
    print(M.to)
    print(M.data)

    msg = '{"TO": "you", "DATA": ["2014-06-13-21:58:53", 7, {"cmd": "adc", "data": [424, 391, 378, 378, 426]}], "FROM": "192.168.68.202:/dev/ttyUSB0"}'
    M.decode(msg)
    print(M.from_host)
    print(M.to)
    print(M.data)

if __name__ == "__main__":
    main()