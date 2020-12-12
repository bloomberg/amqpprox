const spawnSync = require('child_process').spawnSync;
const fs = require('fs');
const http = require('http');

var queuesFile = process.argv[2];
var runIdentifier = process.argv[3];
var localServer = process.argv[4];
var remoteServer = process.argv[5];
var encodedVhost = process.argv[6];
var localPort = process.argv[7];

var counter = 0;
/*
console.info("Run id: ", runIdentifier,
             " localServer: ", localServer,
             " remoteServer: ", remoteServer,
             " encodedVhost: ", encodedVhost, " localPort: ", localPort);
*/
var queuesInfo = JSON.parse(fs.readFileSync(queuesFile, 'utf8'));
for (var i in queuesInfo) {
    var q = queuesInfo[i];
    if (q.messages > 0) {
        console.info("Shovel: " + q.name + " messages: " + q.messages);
        startShovel(q.name);
    }
}

/*

   [  
   {  
   "node":"rabbit@gabrieleMacBook",
   "timestamp":"2015-06-02 15:34:27",
   "name":"test",
   "vhost":"/",
   "type":"dynamic",
   "state":"running",
   "definition":{  
   "src-queue":"test",
   "dest-queue":"test2"
   },
   "src_uri":"amqp://xxxxxxx",
   "dest_uri":"amqp://xxxxxxx"
   }
   ]

 */

checkShovels(function() {
    console.info('Completed shoveling');
});

function startShovel(queuename) {
    // TODO should this be remote not local and pull?
    counter += 1;
    var pathString = '/api/parameters/shovel/'+ encodedVhost + '/' + runIdentifier + '_' + counter;

    var options = {
        host: 'localhost',
        port: localPort,
        auth: 'guest:guest',
        path: pathString,
        method: 'PUT',
        headers: {
            'content-type':'application/json'
        }
    };

    var req = http.request(options, function(res) {

        res.setEncoding('utf8');
        res.on('data', function (chunk) {
            //console.log('BODY: ' + chunk);
        });
    });
    req.on('error', function(e) {
          console.log('problem with request: ' + e.message);
    });

    var request = {
        "value": {
            "src-uri":  localServer,
            "src-queue":  queuename,
            "dest-uri": remoteServer,
            "dest-queue": queuename,
            "delete-after": 'queue-length'
        }
    };
    // write data to request body
    var jsonRequest = JSON.stringify(request);

    req.write(jsonRequest);
    req.end();
}

function checkShovels(completion) {
    var options = {
        host: 'localhost',
        port: localPort,
        auth: 'guest:guest',
        path: '/api/shovels'
    };

    http.get(options, function(rs) {
        var result = "";
        rs.on('data', function(data) {
            result += data;
        });
        rs.on('end', function() {
            //console.log(result);
            var shovels = JSON.parse(result);
            var count = 0;
            for (var i in shovels) {
                var shovel = shovels[i];
                if (shovel.name.startsWith(runIdentifier)) {
                    count++;
                }
            }
            if (count == 0) {
                console.info('No shovels for runId: ', runIdentifier, ' remaining');
                completion();
            }
            else {
                console.info('' + count + ' shovels for runId: '
                             + runIdentifier + ' remaining');
                setTimeout(function() {checkShovels(completion);}, 100);
            }
        });
    });
}
