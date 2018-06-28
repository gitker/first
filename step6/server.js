var static = require('node-static');
var http = require('http');
var file = new(static.Server)();
var https = require('https');
var fs = require('fs');

var options = {
  key: fs.readFileSync('private.pem', 'utf8'),
  cert: fs.readFileSync('csr.crt', 'utf8')
};

var app = https.createServer(options,function (req, res) {
  file.serve(req, res);
}).listen(8080);

var io = require('socket.io').listen(app);
io.sockets.on('connection', function (socket){

	function log(){
		var array = [">>> Message from server: "];
	  for (var i = 0; i < arguments.length; i++) {
	  	array.push(arguments[i]);
	  }
	    socket.emit('log', array);
	}

	socket.on('message', function (message) {
		log('Got message: ', message);
    // For a real app, should be room only (not broadcast)
		socket.broadcast.emit('message', message);
	});

	socket.on('create or join', function (room) {
		var numClients 
		io.of('/').in(room).clients(function(error,clients){
			 numClients=clients.length;
	});

		log('Room ' + room + ' has ' + numClients + ' client(s)');
		log('Request to create or join room', room);

		if (numClients == 0){
			socket.join(room);
			socket.emit('created', room);
		} else if (numClients == 1) {
			io.sockets.in(room).emit('join', room);
			socket.join(room);
			socket.emit('joined', room);
		} else { // max two clients
			socket.emit('full', room);
		}
		socket.emit('emit(): client ' + socket.id + ' joined room ' + room);
		socket.broadcast.emit('broadcast(): client ' + socket.id + ' joined room ' + room);

	});

});
