*** Settings ***
Library         OperatingSystem
Library         Process

*** Keywords ***
Start AMQPClient
    [Arguments]    ${name}
    ${ACCEPTANCE_PATH}=  Get Environment Variable   ACCEPTANCE_PATH
    ${result}=  Start Process  python3.8
    ...                        ${ACCEPTANCE_PATH}/libs/amqp_client/${name}.py
    ...                        stdout=/tmp/logs/amqp_client_${name}.log
    ...                        stderr=/tmp/logs/amqp_client_${name}.log

AMQPClient has log
    [Arguments]    ${name}  ${content}
    ${logContent}=    Get File    /tmp/logs/amqp_client_${name}.log
    Should Contain  ${logContent}  ${content}

AMQPClient print logs
    [Arguments]    ${name}
    ${content}=    Get File    /tmp/logs/amqp_client_${name}.log
    Log  "[AMQPClient] ${name} logs ========="  console=yes
    Log  ${content}  console=yes
    Log  "[AMQPClient] ${name} logs ends ===="  console=yes

