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
		
		socket.broadcast.emit('message', message);
	});

	socket.on('disconnect', function (user) {
    // 从房间名单中移除
    if(roomInfo[roomID]){
			roomInfo[roomID].delete(user);
		}
		if(roomInfo[roomID].size == 0){
			roomInfo[roomID]=null;
		}
     
    socket.leave(roomID);    // 退出房间
		io.sockets.to(roomID).emit('sys', user + '退出了房间');
   
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

