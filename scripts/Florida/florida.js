var http = require('http');
var url = require('url');
var fs = require('fs');

// Settings
var port = process.env.DYNOMITE_FLORIDA_PORT ?
    process.env.DYNOMITE_FLORIDA_PORT : 8080;

var apiUrl = process.env.DYNOMITE_FLORIDA_REQUEST ?
    process.env.DYNOMITE_FLORIDA_REQUEST : '/REST/v1/admin/get_seeds';

// Parse command line options
var seedsFilePath = process.argv[2] && process.argv[2].length > 0 ?
        process.argv[2] : '/etc/dynomite/seeds.list';
var enableDebug = process.argv[3] === 'debug' ? true : false;

http.createServer(function(req, res) {
  var path = url.parse(req.url).pathname;
  enableDebug && console.log('Request: ' + path);

  res.writeHead(200, {'Content-Type': 'application/json'});
  if (path === apiUrl) {
    fs.readFile(seedsFilePath, 'utf-8', function(err, data) {
      if (err) console.log(err); 

      var now = (new Date()).toJSON();
      var seeds = data.trim().replace(/\n/g, '|');

      enableDebug && console.log(now + ' - get_seeds [' + seeds + ']');
      res.write(seeds);
      res.end();
    });
  } else {
    res.end();
  }
}).listen(port);

console.log('Server is listening on ' + port);
