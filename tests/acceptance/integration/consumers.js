const amqp = require('amqp-connection-manager');
const testUtil = require('./testutil');
const wait = require('amqp-connection-manager/lib/helpers').wait;

exports.consumerSet = function (matchFunction, specs, expectingDisconnects) {
    var consumers = [];
    var consumerId = 0;

    var tries = 0;

    var startConsumers = function () {
        for (var specIndex in specs) {
            var spec = specs[specIndex];
            let cons = startConsumer(consumerId, spec.uri, spec.queueName);
            consumers[consumerId] = cons;

            cons.connection.on('connect', function() {
                cons.connected = true;
            });

            consumerId++;
        }
    };

    var checkStarted = function () {
        for (var consIndex in consumers) {
            if (consumers[consIndex].connected == false) {
                return false;
            }
        }
        return true;
    };

    var startConsumer = function (consumerId, uri, queueName) {
        // Handle an incomming message.
        var onMessage = function(data) {
            var message = JSON.parse(data.content.toString());
            matchFunction(message);
            channelWrapper.ack(data);
        }

        var connection = amqp.connect([uri], {json: true});

        connection.on('connect', function() {
            console.log('Connected Consumer:', consumerId);
        });

        connection.on('disconnect', function(params) {
            console.log('Disconnected Consumer: ', consumerId, ' ', params);
            if (!expectingDisconnects()) {
                testUtil.testFailure("Unexpected disconnect for consumer: ",
                                     consumerId);
            }
        });

        // Set up a channel listening for messages in the queue.
        var channelWrapper = connection.createChannel({
            setup: function(channel) {
                return Promise.all([
                    channel.assertQueue(queueName, {durable: true}),
                    channel.prefetch(500),
                    channel.consume(queueName, onMessage)
                ]);
            }
        });

        channelWrapper.waitForConnect()
        .then(function() {
            console.log("Listening for messages, consumer: ", consumerId);
        });

        return {
            'id': consumerId,
            'queueName': queueName,
            'connection': connection,
            'channel': channelWrapper,
            'connected': false
        };
    }

    return {
        start: startConsumers,
        started: checkStarted
    };
};

