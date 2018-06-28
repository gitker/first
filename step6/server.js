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

var roomInfo = {};

io.sockets.on('connection', function (socket){

	function log(){
		var array = [">>> Message from server: "];
	  for (var i = 0; i < arguments.length; i++) {
	  	array.push(arguments[i]);
	  }
	    socket.emit('log', array);
	}

	
	var	roomID = "foo";
	

	socket.on('message', function (message) {
		log('Got message: ', message);
		// For a real app, should be room only (not broadcast)

		if(message.type =='bye'){
			log('disconnect: ', message.user);
			if( roomInfo[roomID]){
				roomInfo[roomID].delete(message.user);
			}
			socket.leave(roomID);  
		}
		
		socket.broadcast.emit('message', message);
	});

	

	socket.on('create or join', function (user) {
		//var numClients = io.of('/').in(room).clients.length;
		if (!roomInfo[roomID]) {
      roomInfo[roomID] = new Set();
		}
		
		if(roomInfo[roomID].size == 0){
			roomInfo[roomID].add(user);
			socket.join(roomID);
			socket.emit('created', roomID);
		}else if(roomInfo[roomID].size == 1){
			socket.join(roomID);
			io.sockets.to(roomID).emit('join', user);
			socket.emit('joined', user);
		}else{
			socket.emit('full', roomID);
		}


	});

});

