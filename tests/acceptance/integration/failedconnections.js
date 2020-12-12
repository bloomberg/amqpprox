const amqp = require('amqp-connection-manager');
const testUtil = require('./testutil');
const wait = require('amqp-connection-manager/lib/helpers').wait;

exports.failedConnectionSet = function (specs) {
    var id = 0;

    var start = function () {
        for (var specIndex in specs) {
            var spec = specs[specIndex];

            for (var i = 0; i < spec.count; ++i, ++id) {
                startFailedConnection(id,
                                      spec.uri,
                                      spec.queueName,
                                      spec.waitTime);
            }
        }
    };

    function startFailedConnection(id, uri, queueName, waitTime) {
        let connection = amqp.connect([uri], {json: true});

        connection.on('connect', function() {
            testUtil.testFailure("Unexpectedly connected for ",
                                 "failedConnectionSet: ", id);
        });

        connection.on('disconnect', function(params) {
            // Expected to hit here
        });
    }

    return {
        "start": start
    };
};
