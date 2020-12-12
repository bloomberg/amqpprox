*** Settings ***
Library         OperatingSystem
Library         Process

*** Keywords ***
Start RabbitMQ Consumer
    [Arguments]    ${name}
    ${ACCEPTANCE_PATH}=  Get Environment Variable   ACCEPTANCE_PATH
    ${result}=  Start Process  python3.8
    ...                        ${ACCEPTANCE_PATH}/libs/rabbitclient/consumer.py
    ...                        stdout=/tmp/logs/consumer_${name}.log
    ...                        stderr=/tmp/logs/consumer_${name}.log

RabbitMQ Consumer has log
    [Arguments]    ${name}  ${content}
    ${logContent}=    Get File    /tmp/logs/consumer_${name}.log
    Should Contain  ${logContent}  ${content}

Print RabbitMQ Consumer
    [Arguments]    ${name}
    ${content}=    Get File    /tmp/logs/consumer_${name}.log
    Log  "[RabbitMQ Consumer] ${name} logs ========="  console=yes
    Log  ${content}  console=yes
    Log  "[RabbitMQ Consumer] ${name} logs ends ===="  console=yes

