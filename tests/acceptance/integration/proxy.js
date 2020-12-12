const fs = require('fs');
const spawn = require('child_process').spawn;
const spawnSync = require('cross-spawn').spawn;
const testUtil = require('./testutil');

var proxyProcess = undefined;
var proxyExecutable = undefined;
var controlExecutable = undefined;
var controlPath = undefined;

exports.setControlPath = function(path) {
    controlPath = path;
};

exports.setExecutables = function (proxyExec, controlExec) {
    proxyExecutable = proxyExec;
    controlExecutable = controlExec;
};

exports.testProxyUp = function () {
    var tries = 0;

    return function () {
        if (proxyExecutable == "-") {
            return true;
        }

        try {
            var controlFile = fs.statSync(controlPath);
            return true;
        }
        catch (err) {
            tries++;

            if (tries > 100) {
                testUtil.testFailure("amqpprox start up timeout");
            }
        }

        return false;
    };
};

exports.startProxy = function () {
    if (proxyExecutable != "-") {
        proxyProcess = spawn(proxyExecutable,
                             ["--cleanupIntervalMs", "10",
                              "--controlSocket", controlPath]);

        proxyProcess.stdout.on('data', function(data) {
            var prefixData = data.toString().split("\n").join("\nstdout: ");
            console.log("stdout: " + prefixData);
        });

        proxyProcess.stderr.on('data', function(data) {
            var prefixData = data.toString().split("\n").join("\nstderr: ");
            console.log("stderr: " + prefixData);
        });

        proxyProcess.on('close', function(code) {
            console.log("child process exited with code " + code);
            testUtil.testFailure("amqpprox exited");
        });
    }
};

exports.runSync = function (commandArgs) {
    commandArgs.splice(0, 0, controlPath);
    var resp = spawnSync(controlExecutable, commandArgs);
}

