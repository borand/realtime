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

if __name__ == "__main__":
    #print(get_host_ip())
    get_python_process_info()