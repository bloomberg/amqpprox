const proxy = require('./proxy');
const testUtil = require('./testutil');

var received = false;

exports.statSender = function() {
    var received = false;

    var startSending = function() {
        var timeoutFunc;
        console.log("Starting to send stats");

        var dgram = require('dgram');
        var sock = dgram.createSocket("udp4");
        sock.on("message", function (message, remote) {
            // Check one well known message
            if (message.toString('utf-8').match(
                    /amqpprox.pausedConnectionCount,rmqEndpointType=overall:[0-9]+|g/g)) {
                clearTimeout(timeoutFunc);
                received = true;
            }
        });

        sock.on("error", function (err) {
            testUtil.testFailure("error listening for stats:\n" + err.stack);
            sock.close();
        });

        timeoutFunc = setTimeout(function() {
            testUtil.testFailure("did not get any stats in 5 seconds\n");
        }, 5000);

        //start the UDP server with the radar port 12345
        sock.bind(6666);
        proxy.runSync(["stat", "send", "localhost", "6666"])
    };

    var haveReceivedStats = function () {
        return received;
    };

    return {
        start: startSending,
        received: haveReceivedStats,
    };
};
