#!/usr/bin/env nodejs

// Copyright (c) 2015. Al Poole <netstar@gmail.com>
//
// http://haxlab.org
//

var http = require('http');

function respondCode(res, val)
{
    res.writeHead(200, {'Content-Type' : 'text/plain'});
    res.write("status: " + val + "\r\n");
    res.end();
}

function basicError(err)
{
    if (err) {
        console.log("basicError");	
    }
}

function initialise()
{

}

http.createServer(function (req, res) {
    var fs = require('fs');
    var username = req.headers['username'];
    var password = req.headers['password'];
    var action   = req.headers['action'];
    var filename = req.headers['filename'];
    var directory = req.headers['directory'];
    var contentLength = req.headers['content-length'];

    if (username == null || username == "") {
         respondCode(res, 0x0005);
    }

    if (password == null || password == "") {
          respondCode(res, 0x0005);
    }

    if (filename != null && filename != "" 
		&& directory != null && directory != "") {
        var path = directory + "/" + filename;
        // wuick fix
        if (!fs.existsSync(directory)) {
                fs.mkdir(directory, [, 0755]);
        }

        var outFile = fs.createWriteStream(path);
       	req.pipe(outFile);
    }

    var len = '';
    req.on('data', function(chunk) {
        len += chunk.length;
    });

    req.on('end', function() { 
	// create your own login here ! no nasty SQL stuff
	// default is "username" and "password" for username and password
	if (username != "username" && password != "password") {
		respondCode(res, 0x01);
        } else {
            respondCode(res, 0x00);
        }
		
        if (action === "ADD") {
            if (!fs.existsSync(directory)) {
                fs.mkdir(directory, [, 0755]);
            }
        } 
        if (filename != null && directory != null && action != null) {
        if (action === "DEL") {
            var path = directory + "/" + filename;
            fs.unlink(path, basicError);
        }
       }
});

}).listen(23221);

console.log("Copyright (c) 2015. Al Poole <netstar@gmail.com>");
console.log("Sanctoshare Server Running!");

