exports.testFailure = function (reason) {
    console.log("Test failed because: ", reason);
    process.exit(1);
}

exports.testSuccess = function () {
    console.log("Test success.");
    process.exit(0);
}

exports.runTests = function (testStages) {
    var currentTestStage = 0;

    function testTimer() {
        var stage = testStages[currentTestStage];

        if (stage.completed()) {
            ++currentTestStage;

            if (currentTestStage >= testStages.length) {
                return;
            }

            stage = testStages[currentTestStage];
            stage.test();
        }

        setTimeout(testTimer, 100);
    }

    // Kick it off
    testStages[0].test();
    testTimer();
};

