from __future__ import print_function

from simplejson import dumps

class Message(object):
    """
    Class for defining validating and handling messages send between system components
    """

    def __init__(self, from_host, to='', data=''):
        self.from_host = from_host
        self.to        = to
        self.data      = data

    def __str__(self):
        return "Message(FROM: %s, TO: %s, DATA: %s)" % (self.from_host, self.to, str(self.data))

    def as_jsno(self):
        data = {"FROM" : self.from_host, "TO" : self.to, "DATA" : self.data}
        return dumps(data)


def main():
    M = Message("192.168.1.10:/dev/ttyUSB0","ALL", [])
    print(M)

    print(M.from_host)
    print(M.to)
    print(M.data)

if __name__ == "__main__":
    main()