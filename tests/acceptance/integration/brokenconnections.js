const amqp = require('amqp-connection-manager');
const net = require('net');
const testUtil = require('./testutil');

exports.brokenConnectionSet = function (specs) {
    var id = 0;

    var startServing = function () {
        const server1 = net.createServer(connectionWaiting);
        server1.listen(5900, () => {
            console.log('5900 server bound');
        });

        const server2 = net.createServer(connectionWaiting);
        server2.listen(5901, () => {
            console.log('5901 server bound');
        });

        const server3 = net.createServer(connectionWaiting);
        server3.listen(5902, () => {
            console.log('5902 server bound');
        });
    };

    var start = function () {
        for (var specIndex in specs) {
            var spec = specs[specIndex];

            for (var i = 0; i < spec.count; ++i, ++id) {
                    startBrokenConnection(id, spec.uri);
            }
        }
    };

    function connectionWaiting(c) {
        // 'connection' listener
        console.log('client connected: '
                    + c.remoteAddress + ':' + c.remotePort + ' -> '
                    + c.localAddress + ':' + c.localPort);
        c.on('end', () => {
            console.log('client disconnected'
                    + c.remoteAddress + ':' + c.remotePort + ' -> '
                    + c.localAddress + ':' + c.localPort);
        });
        var randomizedClose = Math.floor(Math.random() * 10000) + 1;
        setTimeout(function() {
            console.log('close early after ' + randomizedClose + 'ms for '
                    + c.remoteAddress + ':' + c.remotePort + ' -> '
                    + c.localAddress + ':' + c.localPort);
            c.destroy();
        }, randomizedClose);
    };

    function startBrokenConnection(id, uri) {
        let connection = amqp.connect([uri], {json: true});

        var randomizedKill = Math.floor(Math.random() * 10000) + 1;
        setTimeout(function () {
            console.log('Kill the connection early after ' + randomizedKill + 'ms');
            connection.close();
        }, randomizedKill);

        connection.on('connect', function() {
            testUtil.testFailure("Unexpectedly connected for ",
                                 "failedConnectionSet: ", id);
        });

        connection.on('disconnect', function(params) {
            // Expected to hit here, do nothing
        });
    }
    return {
        "startServing": startServing,
        "start": start
    };
}
