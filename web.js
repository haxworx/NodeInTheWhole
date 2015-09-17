var http = require('http');

function respondCode(res, val)
{
	res.writeHead(200, {'Content-Type' : 'text/plain'});
	res.write("status: " + val + "\r\n");
	console.log("it is " + val);
	res.end();
}


function basicError(err)
{
	if (err)	
	{
		console.log("readError");	
	}
}

http.createServer(function (req, res) {
	var contentLength = req.headers['content-length'];
	contentLength = 1024;
	var buffer = new Buffer(contentLength);
	
	var fs = require('fs');

//
//	if (contentLength != 0)
//	{
//		fs.read(process.stdin.fd, buffer, 0, buffer.length, 0, basicError); 
//	}	

	var username = req.headers['username']; // Username'];
	var password = req.headers['password'];
	var action =   req.headers['action'];

	if (username == null || username == "" || password == null || password == "")
	{
		respondCode(res, 0x0005);	
	}

	if (username === "al" && password === "lea")
	{
	}

	else
	{
		respondCode(res, 0x0001);
		return;
	}

	var filename = req.headers['filename'];
	var path = username + "/" + filename;
	// check for password via SQL interaction
	var directory = username;
	directory = "al";
	if (!fs.existsSync(directory))
	{
		fs.mkdir(directory, [, 0755]);
	}

	if (action == "ADD")
	{
		fs.WriteFile(path, buffer, basicError);
	} else if (action == "DEL")
	{
		fs.unlink(path, basicError);
	}

	respondCode(res, 0x0000);
}).listen(8080, "127.0.0.1");

console.log("Web Server Running!");
