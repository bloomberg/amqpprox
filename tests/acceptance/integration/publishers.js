const amqp = require('amqp-connection-manager');
const testUtil = require('./testutil');
const wait = require('amqp-connection-manager/lib/helpers').wait;

exports.publisherSet = function (startupTime,
                                 specs,
                                 expectingDisconnects) {
    var publishers = [];
    var publisherWatermark = 0;

    var getPublisher = function(id) {
        return publishers[id];
    };

    var startPublishers = function () {
        for (var specIndex in specs) {
            var spec = specs[specIndex];

            for (var i = 0; i < spec.count; ++i, ++publisherWatermark) {
                let pub = startPublisher(publisherWatermark,
                                         spec.uri,
                                         spec.queueName,
                                         spec.waitTime);
                publishers[publisherWatermark] = pub;
                pub.connection.on('connect', function () {
                    pub.connected = true;
                });
                pub.send();
            }
        }
    }

    var checkStarted = function () {
        for (var pubIndex in publishers) {
            if (publishers[pubIndex].connected == false) {
                console.log("Publisher: " + pubIndex + " not connected");
                return false;
            }
        }
        return true;
    };

    var printStats = function () {
        for (var pubIndex in publishers) {
            console.log("Publisher " + pubIndex + " messages received: "
                        + publishers[pubIndex].lastReceivedIndex);
        }
    };

    function startPublisher(publisherId, uri, queueName, waitTime) {
        var connection = amqp.connect([uri], {json: true});

        connection.on('connect', function() {
            console.log('Connected Publisher: ', publisherId);
        });

        connection.on('disconnect', function(params) {
            console.log('Disconnected Publisher: ', publisherId, ' ',
                        params.err.stack);

            if (!expectingDisconnects()) {
                testUtil.testFailure("Unexpected disconnect for publisher: " +
                                     publisherId);
            }
        });

        // Create a channel wrapper
        var channelWrapper = connection.createChannel({
            json: true,
            setup: function(channel) {
                return channel.assertQueue(queueName, {durable: true});
            }
        });

        console.log('Reinit messageIndex');
        var messageIndex = 0;
        // Send messages until someone hits CTRL-C or something goes wrong...
        var sendMessage = function() {
            messageIndex++;
            channelWrapper.sendToQueue(queueName,
                {
                    time: Date.now(),
                    messageIndex:messageIndex,
                    publisherId:publisherId,
                    startupTime:startupTime
                })
            .then(function() {
                return wait(waitTime);
            })
            .then(function() {
                return sendMessage();
            }).catch(function(err) {
                return console.log("Message was rejected:", err.stack);
                channelWrapper.close();
                connection.close();
            });
        };

        return {
            'id': publisherId,
            'queueName': queueName,
            'connection': connection,
            'channel': channelWrapper,
            'send' : sendMessage,
            'lastReceivedIndex': 0,
            'connected': false
        };
    }

    return {
        "start": startPublishers,
        "started": checkStarted,
        "getPublisher": getPublisher,
        "printStats": printStats
    };
}
