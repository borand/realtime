from fabric.api import run

__author__ = 'andrzej'


def host_type():
    run('uname -s')

def start_all():
    venv = '~/projects/realtime/venv/bin/python'
    cmd  = '~/projects/realtime/rtweb/rtweb.py'

    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    cmd = '~/projects/realtime/rtweb/comport.py run --redishost 127.0.0.1'
    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    cmd = '~/projects/realtime/rtweb/daq_irq.py'
    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    venv = ''
    cmd = '~/realtime/venv/bin/rqworker'
    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

