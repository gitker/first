'use strict';

var isChannelReady;
var isInitiator = false;
var isStarted = false;
var localStream;
var pc;
var remoteStream;
var turnReady;

//var pc_config = {'iceServers': [{'url': 'stun:stun.freeswitch.org'},
//{'url': 'stun:stun.xten.com'},{'url': 'stun:stun.ekiga.net'}
//]};

var pc_config ={'iceServers': [
  {'url': 'stun:stun.freeswitch.org'},
  {'url': 'stun:stun.xten.com'},
  {'url': 'stun:stun.ekiga.net'},
  {
    urls: "turn:118.24.135.37", 
    credential: "ling1234", 
    username: "ling"
  }
]};


var pc_constraints = {'optional': [{'DtlsSrtpKeyAgreement': true}]};

var constraints = {video: true,audio:true};

// Set up audio and video regardless of what devices are present.
var sdpConstraints = {'mandatory': {
  'OfferToReceiveAudio':true,
  'OfferToReceiveVideo':true }};

/////////////////////////////////////////////


var usr = new Date().toLocaleString( );

var socket = io.connect();


socket.on('full', function (room){
  alert('Room ' + room + ' is full');
});

socket.on('join', function (user){
  console.log('Another peer made a request to join  ' + user);
  isChannelReady = true;
  maybeStart();
});

socket.on('joined', function (user){
  console.log('This peer has joined user ' + user);
  isChannelReady = true;
});

socket.on('log', function (array){
  console.log.apply(console, array);
});

////////////////////////////////////////////////

function sendMessage(message){
	console.log('Client sending message: ', message);
  // if (typeof message === 'object') {
  //   message = JSON.stringify(message);
  // }
  socket.emit('message', message);
}

socket.on('message', function (message){
  console.log('Client received message:', message);
  if (message.type === 'offer') {
    if(!createPeerConnection()){
      return;
    }
    pc.setRemoteDescription(new RTCSessionDescription(message));
    doAnswer();
  } else if (message.type === 'answer'&& isStarted ) {
    pc.setRemoteDescription(new RTCSessionDescription(message));
  } else if (message.type === 'candidate' && isStarted)  {
    var candidate = new RTCIceCandidate({
      sdpMLineIndex: message.label,
      candidate: message.candidate
    });
    pc.addIceCandidate(candidate);
  } else if (message.type === 'bye')  {
   stop();
  }
});

////////////////////////////////////////////////////

var localVideo = document.querySelector('#localVideo');
var remoteVideo = document.querySelector('#remoteVideo');

function handleUserMedia(stream) {
  console.log('Adding local stream.');
  localVideo.src = window.URL.createObjectURL(stream);
  localStream = stream;
  socket.emit('join', usr);
}

function handleUserMediaError(error){
  alert('getUserMedia error: '+ error);
}


getUserMedia(constraints, handleUserMedia, handleUserMediaError);

console.log('Getting user media with constraints', constraints);


function maybeStart() {
    if (!createPeerConnection())
       return false;
    isStarted = true;
    doCall();
}

window.onbeforeunload = function(e){
  sendMessage({type:'bye',user:usr});
}

/////////////////////////////////////////////////////////

function createPeerConnection() {
  if (pc)
  return true;
  try {
    pc = new RTCPeerConnection(pc_config);
    pc.onicecandidate = handleIceCandidate;
    pc.onaddstream = handleRemoteStreamAdded;
    pc.onremovestream = handleRemoteStreamRemoved;
    pc.addStream(localStream);
    console.log('Created RTCPeerConnnection');
    return  true;
  } catch (e) {
    console.log('Failed to create PeerConnection, exception: ' + e.message);
    hangup();
    alert('Cannot create RTCPeerConnection object.' + e.message);
    return false;
  }
}

function handleIceCandidate(event) {
  console.log('handleIceCandidate event: ', event);
  if (event.candidate) {
    sendMessage({
      type: 'candidate',
      label: event.candidate.sdpMLineIndex,
      id: event.candidate.sdpMid,
      candidate: event.candidate.candidate});
  } else {
    console.log('End of candidates.');
  }
}

function handleRemoteStreamAdded(event) {
  console.log('Remote stream added.');
  remoteVideo.src = window.URL.createObjectURL(event.stream);
  remoteStream = event.stream;
}

function handleCreateOfferError(e){
  console.log('createOffer() error: ', e);
}

function handleCreateAnswerError(e){
  console.log('createAnswer() error: ', e);
}

function doCall() {
  console.log('Sending offer to peer');
  pc.createOffer(setLocalAndSendMessage, handleCreateOfferError);
}

function doAnswer() {
  console.log('Sending answer to peer.');
  pc.createAnswer(setLocalAndSendMessage, handleCreateAnswerError, sdpConstraints);
}

function setLocalAndSendMessage(sessionDescription) {
  // Set Opus as the preferred codec in SDP if Opus is present.
 // sessionDescription.sdp = preferOpus(sessionDescription.sdp);
  pc.setLocalDescription(sessionDescription);
  console.log('setLocalAndSendMessage sending message' , sessionDescription);
  sendMessage(sessionDescription);
}


function handleRemoteStreamAdded(event) {
  console.log('Remote stream added.');
  remoteVideo.src = window.URL.createObjectURL(event.stream);
  remoteStream = event.stream;
  localVideo.hidden = true;
}

function handleRemoteStreamRemoved(event) {
  console.log('Remote stream removed. Event: ', event);
}

function hangup() {
  console.log('Hanging up.');
  sendMessage({type:'bye',user:usr});
  stop();
}


function stop() {
  isStarted = false;
  // isAudioMuted = false;
  // isVideoMuted = false;
  localVideo.hidden = false;
 if(pc)
   pc.close();
  pc = null;
}
