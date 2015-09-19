#!/usr/bin/env nodejs

// Copyright (c) 2015. Al Poole <netstar@gmail.com>
//
// http://haxlab.org
//

var http = require('http');
var mysql = require('mysql');

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
    var db = mysql.createConnection({
	host: '127.0.0.1',
        user: 'matthias',
        password: 'z6srkrmt',
        database: 'santoshare',
    });

    return db;
}

http.createServer(function (req, res) {
    var fs = require('fs');
    var username = req.headers['username'];
    var password = req.headers['password'];
    var action   = req.headers['action'];
    var filename = req.headers['filename'];
    var contentLength = req.headers['content-length'];

    if (username == null || username == "") {
         respondCode(res, 0x0005);
    }

    if (password == null || password == "") {
          respondCode(res, 0x0005);
    }

    if (filename != null && filename != "") {
        var path = username + "/" + filename;
        var directory = username;
        var outFile = fs.createWriteStream(path);
       	req.pipe(outFile);
    }

    var len = '';
    req.on('data', function(chunk) {
        len += chunk.length;
    });

    req.on('end', function() {
        var db = initialise();

        var SQL = "SELECT password FROM users WHERE username = ? AND active = 1";

        var query = db.query(SQL, [username], function(err, result) {
        if (err) {
           throw err;
           respondCode(res, 0x01);
        }
			
        if (! result || result.length == 0) {
            respondCode(res, 0x01);
        }	

        var users_password = result[0].password;			
        if (! users_password) {
            respondCode(res, 0x02);
        }

        if (password !== users_password) {
            respondCode(res, 0x01);
        } else {
            respondCode(res, 0x00);
        }
		
        if (action === "ADD") {
            if (!fs.existsSync(directory)) {
                fs.mkdir(directory, [, 0755]);
            }
        //fs.writeFileSync(path, buffer);
        } else if (action === "DEL") {
             fs.unlink(path, basicError);
        }
    });
});

}).listen(8080);

console.log("Copyright (c) 2015. Al Poole <netstar@gmail.com>");
console.log("Sanctoshare Server Running!");

