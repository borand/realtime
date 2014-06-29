import datetime

import simplejson as sjson

from logbook import Logger
from requests import get
from redis import Redis

log = Logger('datastore')

def get_last_value(serial_number):
    R = Redis()
    datavalue_json = R.get('serial_number:'+serial_number)
    if datavalue_json is not None:
        try:
            datavalue_obj = sjson.loads(datavalue_json)
            timestamp = datavalue_obj[0]
            datavalue = datavalue_obj[1]
        except Exception as e:
            log.error("Exception occured, within get_last_value function: %s" % E.message)
            return None
        
        timestamp = datetime.datetime.strptime(timestamp.split('.')[0],"%Y-%m-%d-%H:%M:%S")
        return {'serial_number' : serial_number, 'timestamp': timestamp, 'datavalue' : datavalue}
    else:
        return None

def save_last_value(serial_number, timestamp, datavalue):
    R = Redis()
    datavalue_json = sjson.dumps([timestamp.strftime('%Y-%m-%d-%H:%M:%S'), datavalue])
    R.set('serial_number:'+serial_number, datavalue_json)

def submit(data_set, timestamp='', submit_to='0.0.0.0', port=8000, threshold=0, max_interval=3600):

    if isinstance(timestamp, str):
        if timestamp.lower() == 'now':
            timestamp = datetime.datetime.now()
        else:
            try:
                timestamp = datetime.datetime.strptime(timestamp.split('.')[0],"%Y-%m-%d-%H:%M:%S") 
            except:
                timestamp = datetime.datetime.now()
    
    try:
        ret = []
        for data in data_set:
            serial_number = data[0]
            datavalue     = data[-1]
            
            if isinstance(datavalue, str):
                url = 'http://%s:%d/sensordata/api/submit/datavalue/%s/sn/%s/val/%s' \
                    % (submit_to, port, timestamp.strftime('%Y-%m-%d-%H:%M:%S'), serial_number, datavalue)
            else:
                last_submitted = get_last_value(serial_number)
                if last_submitted is not None:                
                    time_since_last_submission  = timestamp - last_submitted['timestamp']                
                    
                    if time_since_last_submission.seconds < max_interval and abs(datavalue - last_submitted['datavalue']) < threshold:
                        status_msg = '[SKIPPING], %s value less than %d sec old and below min change threshold %f' \
                                   % (serial_number, max_interval, threshold)
                        ret.append(status_msg)
                        continue
                
                url = 'http://%s:%d/sensordata/api/submit/datavalue/%s/sn/%s/val/%.3f' \
                    % (submit_to, port, timestamp.strftime('%Y-%m-%d-%H:%M:%S'), serial_number, datavalue)

            log.debug('submitting to: %s' % url)
            res = get(url)
            if res.ok:                
                log.info(res.content)
                ret.append(res.content)
                save_last_value(serial_number, timestamp, datavalue)
            else:
                log.info(res)

        return ret

    except Exception as E:
        log.error("Exception occured, within the submit function: %s" % E.message)
        log.error('q_data = %s' % str(data_set[0]))
        return E.message

if __name__ == "__main__":
    submit([['0', 0.0]])
