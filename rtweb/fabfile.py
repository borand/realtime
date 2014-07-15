from fabric.api import run, env

env.hosts = ['localhost']

__author__ = 'andrzej'

ROOT_DIR = '/home/andrzej'


def host_type():
    run('uname -s')

def start_all():
    venv = ROOT_DIR + '/projects/realtime/venv/bin/python'
    cmd  = '~/projects/realtime/rtweb/rtweb.py'

    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    cmd = ROOT_DIR + '/projects/realtime/rtweb/comport.py run --redishost 127.0.0.1'
    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    cmd = ROOT_DIR + '/projects/realtime/rtweb/daq_irq.py'
    full_cmd = 'nohup {0} {1} run --redishost localhsot > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    venv = ''
    cmd = '/home/andrzej/projects/realtime/venv/bin/rqworker'
    full_cmd = 'nohup {0} {1} > /dev/null &'.format(venv,cmd)
    print full_cmd
    run(full_cmd)

    run('ps -ef | grep python')

