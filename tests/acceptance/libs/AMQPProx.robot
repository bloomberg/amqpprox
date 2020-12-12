*** Settings ***
Library         OperatingSystem
Library         Process

*** Keywords ***
AMQPProx start
    Create Directory  /tmp/logs/amqpprox
    ${SOURCE_PATH}=       Get Environment Variable   SOURCE_PATH
    ${BUILD_PATH}=       Get Environment Variable   BUILD_PATH
    ${result}=  Start Process  ${BUILD_PATH}/amqpprox/amqpprox --cleanupIntervalMs 10 --controlSocket /tmp/amqpprox --logDirectory /tmp/logs/amqpprox
    ...                      shell=yes
    [Return]    ${result}

AMQPProx stop
    [Arguments]  ${handle}
    ${result} =  Terminate Process  ${handle}

AMQPProx print logs
    [Documentation]     Only for debug purposes
    ${result}=  Run Process  tail -n 120 /tmp/logs/amqpprox/*  shell=yes
    Log  "[AMQPPROX] logs ========"  console=yes
    Log  ${result.stdout}  console=yes
    Log  "[AMQPPROX] logs end ===="  console=yes
