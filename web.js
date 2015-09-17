var http = require('http');

function respondCode(res, val)
{
	res.writeHead(200, {'Content-Type' : 'text/plain'});
	res.write("status: " + val + "\r\n");
	res.end();
}


function basicError(err)
{
	if (err)	
	{
		console.log("basicError");	
	}
}

http.createServer(function (req, res) {
	var contentLength = req.headers['content-length'];
	
	var fs = require('fs');
	var buffer = "";

	req.on('data', function(data) {
		buffer = data.toString();
	});
	
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

	var directory = username;

	if (!fs.existsSync(directory))
	{
		fs.mkdir(directory, [, 0755]);
	}

	if (action === "ADD")
	{
		req.on('end', function () {
			console.log("added " + buffer);
			fs.writeFileSync(path, buffer);
		});
	} else if (action === "DEL")
	{
		fs.unlink(path, basicError);
	}

	respondCode(res, 0x0000);
}).listen(8080, "127.0.0.1");

console.log("Sanctoshare Server Running!");
