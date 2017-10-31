
import socket
import select
import time
import errno
import traceback
import logging
from collections import defaultdict

TIMEOUT_PRECISION = 10
POLL_NULL = 0x00
POLL_IN = 0x01
POLL_OUT = 0x04
POLL_ERR = 0x08
POLL_HUP = 0x10
POLL_NVAL = 0x20

WAIT_STATUS_INIT = 0
WAIT_STATUS_READING = 1
WAIT_STATUS_WRITING = 2
WAIT_STATUS_READWRITING = WAIT_STATUS_READING | WAIT_STATUS_WRITING

HEADER_LEN = 13

STAGE_INT = 0
STAGE_PUT = 1
STAGE_FINISH =2

def errno_from_exception(e):
	if hasattr(e,'errno'):
		return e.errno
	elif e.args:
		return e.args[0]
	else:
		return None

def to_number(data):
	return int(data)

class TCPHandler(object):
	def __init__(self,loop,sock,num):
		self._loop = loop
		self._sock = sock
		self._destroyed = False
		self._data_to_write = []
		self._status = WAIT_STATUS_INIT
		self._directory = "/root/"
		self._stage = STAGE_INT
		self._header = ""
		self._data_appending=[]
		self._data_appending_len = 0
		self._flen = 0
		self._num = num
		self._fd = open("tmp","wb")
		self._data = []
		sock.setblocking(False)
		sock.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
		loop.add(sock,POLL_IN |POLL_ERR,self)

	def _parse_header(self,header):
		if len(header) != HEADER_LEN:
			self.destroy()
			return
		if header[:3] == "PUT":
			print("PUT")
			self._flen = to_number(header[3:])
			print(self._flen)
			self._stage = STAGE_PUT


	def _handle_int(self,data):
		total_len = len(self._header) + len(data)
		if total_len < HEADER_LEN:
			self._header += data
			return
		elif total_len == HEADER_LEN:
			self._header += data
		else:
			last = data[:(HEADER_LEN-len(self._header))]
			rest = data[(HEADER_LEN-len(self._header)):]
			self._header += last
			self._data_appending.append(rest)
			self._data_appending_len = len(rest)
		self._parse_header(self._header)

	def _write_to_file(self,data):
		self._data.append(data)
		self._fd.write(data)
		
		self._flen -= len(data)


	def _handle_put(self,data):
		buf_size = 32* 1024
		print("handle len:",len(data))
		if len(data) + self._data_appending_len < buf_size:
			self._data_appending.append(data)
			self._data_appending_len += len(data)
		else:
			pre = buf_size -  self._data_appending_len 
			post = len(data) - pre
			self._data_appending.append(data[:pre])
			buf = b''.join(self._data_appending)
			self._write_to_file(buf)
			self._data_appending = []
			self._data_appending_len = 0
			if post > 0:
				self._data_appending.append(data[pre:])
				self._data_appending_len = post

		if self._flen == self._data_appending_len:
			self._write_to_file(b''.join(self._data_appending))
			print("finished")
			print(len(b''.join(self._data)))
			self.destroy()





	def _on_read(self):
		if not self._sock:
			return
		data = None
		buf_size = 32 * 1024
		try:
			data = self._sock.recv(buf_size)
		except (OSError,IOError) as e:
			if errno_from_exception(e) in (errno.ETIMEDOUT, errno.EAGAIN, errno.EWOULDBLOCK):
				return
		if not data:
			print("no data")
			self.destroy()
			return
		if self._stage == STAGE_INT:
			self._handle_int(data)
			return
		elif self._stage == STAGE_PUT:
			self._handle_put(data)
			return
		else:
			pass

	def _write_to_sock(self,data,sock):
		if not data or not sock:
			return False
		uncomplete = False
		try:
			l = len(data)
			s = sock.send(data)
			if s<l:
				data = data[s:]
				uncomplete = True
		except (OSError,IOError) as e:
			errno_no = errno_from_exception(e)
			if errno_no in (errno.EAGAIN, errno.EINPROGRESS,errno.EWOULDBLOCK):
				uncomplete = True
			else:
				print("destroy 157")
				self.destroy()
				return False
		if uncomplete:
			self._data_to_write.append(data)
			self._update_status(WAIT_STATUS_WRITING)
		else:
			self._update_status(WAIT_STATUS_READING)
		return True


	def _update_status(self,status):

		if self._status != status:
			self._status = status
		else:
			return
		event = POLL_ERR
		if self._status & WAIT_STATUS_WRITING:
			event |= POLL_OUT
		if self._status & WAIT_STATUS_READING:
			event |= POLL_IN
		self._loop.modify(self._sock,event)



	def _on_write(self):
		if self._data_to_write:
			data = b''.join(self._data_to_write)
			self._data_to_write = []
			self._write_to_sock(data,self._sock)
		else:
			self._update_status(WAIT_STATUS_READING)


	def handle_event(self,sock,fd,event):
		if self._destroyed:
			return
		if sock == self._sock:
			if event & POLL_ERR:
				self.destroy()
				if self._destroyed:
					return
			if event &(POLL_IN | POLL_HUP):
				
				self._on_read()
				if self._destroyed:
					return
			if event & POLL_OUT:
				self._on_write()
				if self._destroyed:
					return

	def destroy(self):
		self._destroyed = True

		if self._sock:
			print("destroy start")
			print(self._num)
			self._loop.remove(self._sock)
			self._sock.close()
			self._fd.close()
			self._fd = None
			self._sock = None


class Server(object):
	def __init__(self,config):
		self._config = config
		self._timeout = 600
		self._loop = None
		listen_addr = '127.0.0.1'
		listen_port = 2121
		self._child = 0

		addrs = socket.getaddrinfo(listen_addr,listen_port,0,socket.SOCK_STREAM, socket.SOL_TCP)

		if len(addrs) == 0:
			raise Exception("con't get addrinfo")
		af, socktype, proto, canonname, sa = addrs[0]
		server_socket = socket.socket(af,socktype,proto)
		server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		server_socket.bind(sa)
		server_socket.setblocking(False)
		server_socket.listen(1024)
		self._server_socket = server_socket

	def add_to_loop(self,loop):
		if self._loop:
			return
		self._loop = loop
		loop.add(self._server_socket,POLL_IN|POLL_ERR,self)

	def handle_event(self,sock,fd,event):
		if sock == self._server_socket:
			if event & POLL_ERR:
				raise Exception('server_socket error')
			try:
				print('accept')
				conn = self._server_socket.accept()
				self._child +=1
				TCPHandler(self._loop,conn[0],self._child)
			except (OSError,IOError) as e:
				pass
	



class EventLoop(object):
	def __init__(self):
		
		self._impl = select.epoll()
		self._fdmap = {}
		self._stopping = False

	def poll(self,timeout=None):
		events = self._impl.poll(timeout)
		return [(self._fdmap[fd][0],fd,event) for fd,event in events]

	def add(self,f,mode,handler):
		fd = f.fileno()
		self._fdmap[fd] = (f,handler)
		self._impl.register(fd,mode)

	def remove(self,f):
		fd = f.fileno()
		del self._fdmap[fd]
		self._impl.unregister(fd)

	def modify(self,f,mode):
		fd = f.fileno()
		self._impl.modify(fd,mode)


	def run(self):
		events = []
		while not self._stopping:
			asap = False
			try:
				events = self.poll(TIMEOUT_PRECISION )
			except (OSError,IOError) as e:
				if errno_from_exception(e) in (errno.EPIPE,errno.EINTR):
					asap = True
					logging.debug('poll:%s', e)
				else:
					logging.error('poll:%s', e)
					traceback.print_exc()
					continue
			for sock , fd , event in events:
				handler = self._fdmap.get(fd,None)
				if handler is not None:
					handler = handler[1]
					try:
						handler.handle_event(sock,fd,event)
					except (OSError,IOError) as e:
						logging.error(e)
						traceback.print_exc()

	def __del__(self):
		self._impl.close()

def main():
	loop = EventLoop()
	server = Server(None)
	server.add_to_loop(loop)
	loop.run()

main()

