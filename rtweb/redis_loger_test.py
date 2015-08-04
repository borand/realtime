from redislog import handlers, logger
l = logger.RedisLogger('loger_test')
l.addHandler(handlers.RedisHandler.to("log", host='localhost', port=6379))

def print_msg(l):
	try:
		l.info("I am a message")	
	except:
		print("Error message")


if __name__ == '__main__':
	l.info("I like pie")
	l.debug("I don't like to make pie")
	l.error("Trousers! - the pie is killing me")
	
	print_msg(l)
	l.error("Trousers! - the pie is killing me")


