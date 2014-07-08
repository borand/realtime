from __future__ import  print_function

__author__ = 'aborowie'

import re
import sh
import psutil

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

def get_python_process_info():
    out = []
    for p in psutil.process_iter():
        cmd = p.cmdline        
        if len(cmd) > 0 and "python" in cmd[0]:
            out.append(p)
            print(p.cmdline)
    return out

def get_process_info(process):
    for p in psutil.process_iter():
        cmd = p.cmdline
        if len(cmd) > 1 and process in cmd[1]:
            print(p)
            return p

def SerialNumberToDec(hex_str, tostr=True):
    dec_vector = []
    str_out = ''
    for i in range(0,len(hex_str),2):
        dec_vector.append(int(float.fromhex(hex_str[i:i+2])))

    if tostr:
        d = dec_vector
        return "{0} {1} {2} {3} {4} {5} {6} {7}".format(d[0],d[1], d[2], d[3], d[4], d[5], d[6], d[7])
    return dec_vector

def SerialNumberToHex(sn):

    tmp = [int(x) for x in re.findall("(\d+)",sn)]

    sn = ''
    if len(tmp) == 8:
        for i in tmp:
            x = hex(i)[2:].upper()
            if len(x) == 1:
                x = '0'+x
            sn+=x
    else:
        pass
    return sn

if __name__ == "__main__":
    #print(get_host_ip())
    get_python_process_info()