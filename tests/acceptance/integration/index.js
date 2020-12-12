const amqp = require('amqp-connection-manager');
const brokenConnectionSet = require('./brokenconnections').brokenConnectionSet;
const consumerSet = require('./consumers').consumerSet;
const failedConnectionSet = require('./failedconnections').failedConnectionSet;
const fs = require('fs');
const proxy = require('./proxy');
const publisherSet = require('./publishers').publisherSet;
const statSender = require('./stats').statSender;
const testUtil = require('./testutil');
const util = require('util');
const wait = require('amqp-connection-manager/lib/helpers').wait;

var controlPath = "/tmp/amqpprox";
var startupTime = Date.now();
var proxyExecutable = process.argv[2];
var controlExecutable = process.argv[3];
var testDuration = process.argv[4];

proxy.setExecutables(proxyExecutable, controlExecutable);
proxy.setControlPath(controlPath);

var failConnectionSpecs = [
    {
        uri: 'amqp://localhost:5555/bar?heartbeat=10',
        queueName: 'amqp-pub3',
        waitTime: 100,
        count: 1000
    },
    {
        uri: 'amqp://localhost:5555/nonexistent?heartbeat=10',
        queueName: 'amqp-pub3',
        waitTime: 100,
        count: 1000
    }
];

var publisherSpecs = [
    {
        uri: 'amqp://localhost:5555/?heartbeat=10',
        queueName: 'amqp-pub1',
        waitTime: 3,
        count: 3
    },
    {
        uri: 'amqp://localhost:5555/foo?heartbeat=10',
        queueName: 'amqp-pub2',
        waitTime: 1000,
        count: 30
    },
];


var consumerSpecs = [
    {
        uri: 'amqp://localhost:5555/?heartbeat=10',
        queueName: 'amqp-pub1'
    },
    {
        uri: 'amqp://localhost:5555/foo?heartbeat=10',
        queueName: 'amqp-pub2'
    },
];

var brokenConnectionSpec = [
    {
        uri: 'amqp://localhost:5555/broken?heartbeat=10',
        count: 1000
    },
    {
        uri: 'amqp://localhost:5555/broken2?heartbeat=10',
        count: 1000
    },
    {
        uri: 'amqp://localhost:5555/broken3?heartbeat=10',
        count: 1000
    },
    {
        uri: 'amqp://localhost:5555/broken4?heartbeat=10',
        count: 1000
    }
];

var consumers = consumerSet(consumeMessage, consumerSpecs, expectingDisconnects);
var publishers = publisherSet(startupTime, publisherSpecs, expectingDisconnects);
var failedconnections = failedConnectionSet(failConnectionSpecs);
var brokenConnections = brokenConnectionSet(brokenConnectionSpec);
var stats = statSender();

var testStages = [
    // Let everyone know we're starting
    { test: startupLog, completed: yes },
    // Get the proxy to the point of having a control socket available
    { test: proxy.startProxy, completed: proxy.testProxyUp() },
    // Synchronously set the mappings up and begin listening
    { test: setupMappings, completed: yes },
    // Test that sending stats over UDP is possible
    { test: stats.start, completed: stats.received },
    // Start the consumers before the publishers
    { test: consumers.start, completed: consumers.started },
    { test: publishers.start, completed: publishers.started },
    // Kick off some connections that fail to establish
    { test: failedconnections.start, completed: yes },
    // Start off some connections that terminate themselves randomly
    { test: brokenConnections.startServing, completed: yes },
    { test: brokenConnections.start, completed: yes },
    // Wait 30 seconds to make sure nothing breaks
    { test: stopLog, completed: yes }
];

testUtil.runTests(testStages);

function consumeMessage(message) {
    if (message.startupTime == startupTime) {
        // drop messages if they weren't from this run of the test
        var publisher = publishers.getPublisher(message.publisherId);
        var lastReceived = publisher.lastReceivedIndex;
        if (message.messageIndex != lastReceived + 1) {
            if (message.messageIndex == lastReceived) {
                console.log("OK Receiver DUPLICATED message ", message,
                            " expected index: ", lastReceived + 1);
            }
            else {
                testUtil.testFailure("Receiver got message ", message,
                                     " expected index: ", lastReceived + 1,
                                     " publisher:", publisher);
            }
        }
        publisher.lastReceivedIndex = message.messageIndex;
    }
}

var setupCommands = [
    ["log", "console", "3"],
    ["backend", "add", "shared1-1", "dc-1", "127.0.0.1", "5800"],
    ["backend", "add", "shared2-1", "dc-2", "127.0.0.1", "5801"],
    ["backend", "add", "shared3-1", "dc-2", "localhost", "5900"],
    ["backend", "add", "shared3-2", "dc-1", "localhost", "5901"],
    ["backend", "add", "shared3-3", "dc-2", "localhost", "5902"],
    ["backend", "add", "shared4-1", "dc-2", "localhost", "5899"],
    ["farm", "add_manual", "shared1", "round-robin", "shared1-1"],
    ["farm", "add_manual", "shared2", "round-robin", "shared2-1"],
    ["farm", "add_manual", "shared3", "round-robin", "shared3-1", "shared3-2", "shared3-3"],
    ["farm", "add_manual", "shared4", "round-robin", "shared3-1", "shared3-2", "shared4-1"],
    ["farm", "add_manual", "shared5", "round-robin"],
    ["farm", "add_manual", "shared6", "round-robin", "shared1-1", "shared-3-1", "shared4-1", "shared3-2"],
    ["map", "farm", "/", "shared1"],
    ["map", "farm", "bar", "shared1"],
    ["map", "farm", "foo", "shared2"],
    ["map", "farm", "broken", "shared3"],
    ["map", "farm", "broken2", "shared4"],
    ["map", "farm", "broken3", "shared5"],
    ["map", "farm", "broken4", "shared6"],
    ["map", "backend", "foobar", "shared1-1"],
    ["datacenter", "set", "dc-2"],
    ["listen", "start", "5555"]
];

function setupMappings() {
    for (var idx in setupCommands) {
        console.log("Execute command: " + setupCommands[idx]);
        proxy.runSync(setupCommands[idx]);
    }
}

function startupLog() {
    console.log("Starting integration test with proxy: ", proxyExecutable,
                " and ctl: ", controlExecutable);
}

function stopLog() {
    var sleep = 30000;
    if (testDuration > 0) {
        sleep = testDuration * 1000;
    }

    setTimeout(function() {
        publishers.printStats();
        testUtil.testSuccess();
    }, sleep);
}

function yes() {
    return true;
}

function expectingDisconnects() {
    return false;
}
