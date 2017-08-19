var net = require('net')
var express = require('express')



var host = 'localhost'
var port = 8123
var s = net.createServer()
var imdata = new Buffer('yesicando')
s.listen(port, host)

s.on('connection', function(sock){
	console.log('new customer: ', sock.remoteAddress, sock.remotePort)
	sock.on('data', function(data){
		//console.log('got ', data.length)//, '\n', data)
		//console.log(data + '')
		imdata = data;
	})
	sock.on('close', function(){
		console.log('ending')
	})
})

var app = express()
app.use(express.static(__dirname))
app.get('/getim', function(req, res){
	res.writeHead(200, {'Content-Type': 'image/jpeg'})
    res.end(imdata)
	//res.send(imdata)
})
app.listen(80)