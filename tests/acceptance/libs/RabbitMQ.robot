*** Settings ***
Library         OperatingSystem
Library         Process

*** Keywords ***
RabbitMQ suite setup
    ${BINARY_PATH}=       Get Environment Variable   BINARY_PATH
    ${LOG_CONSOLE}=       Get Environment Variable   LOG_CONSOLE
    Set suite variable    ${BINARY_PATH}
    Set suite variable    ${LOG_CONSOLE}

Make config directory
    Log  "[RabbitMQ] Making config directory"
    ...  console=${LOG_CONSOLE}
    Create Directory  /etc/rabbitmq

Configure plugins
    Log  "[RabbitMQ] Configuring Plugins"
    ...  console=${LOG_CONSOLE}
    ${result}=  Run Process  ${BINARY_PATH}/rabbitmq-plugins
    ...                      enable
    ...                      --offline
    ...                      rabbitmq_shovel
    ...                      rabbitmq_shovel_management
    [Return]    ${result}

Setting definitions
    [Arguments]    ${port}  ${file}
    Log  "[RabbitMQ] Setting definitions"
    ...  console=${LOG_CONSOLE}
    Run Process  curl -s -u guest:guest -H "content-type:application/json" -X POST http://localhost:${port}/api/definitions -d \@${file}
    ...          shell=True

Make admin user
    [Arguments]    ${nodename}
    Log  "[RabbitMQ] Make admin user"
    ...  console=${LOG_CONSOLE}
    Run Process  ${BINARY_PATH}/rabbitmqctl  -q
    ...          -n  ${nodename}
    ...          add_user  admin  admin
    Run Process  ${BINARY_PATH}/rabbitmqctl  -q
    ...          -n  ${nodename}
    ...          set_user_tags  admin  administrator

Start rabbitmq process and wait
    [Arguments]    ${pid_file}  ${nodename}  ${nodeport}  ${configfile}
    Start rabbitmq process  pid_file=${pid_file}
    ...                     nodename=${nodename}
    ...                     nodeport=${nodeport}
    ...                     configfile=${configfile}
    Wait rabbitmq process  pid_file=${pid_file}
    ...                    nodename=${nodename}

Start rabbitmq process
    [Arguments]    ${pid_file}  ${nodename}  ${nodeport}  ${configfile}
    Log  "[RabbitMQ] Starting RabbitMQ ${nodename}"
    ...  console=${LOG_CONSOLE}
    Should Exist  ${configfile}.config
    ${result}=  Start Process  ${BINARY_PATH}/rabbitmq-server
    ...            env:RABBITMQ_LOG_BASE=/tmp/logs
    ...            env:RABBITMQ_PID_FILE=${pid_file}
    ...            env:RABBITMQ_NODENAME=${nodename}
    ...            env:RABBITMQ_NODE_PORT=${nodeport}
    ...            env:RABBITMQ_CONFIG_FILE=${configfile}
    [Return]  ${result}

RabbitMQ stop
    [Arguments]  ${handle}
    ${result} =  Terminate Process  ${handle}

Wait rabbitmq process
    [Arguments]    ${pid_file}  ${nodename}
    Log  "[RabbitMQ] Waiting for ${nodename}"
    ...  console=${LOG_CONSOLE}
    ${result} =  Run Process  ${BINARY_PATH}/rabbitmqctl
    ...                       -n  ${nodename}
    ...                       wait  ${pidfile}

RabbitMQ has log
    [Arguments]    ${nodename}  ${content}
    ${logContent}=    Get File    /tmp/logs/${nodename}.log
    Should Contain  ${logContent}  ${content}

RabbitMQ print logs
    [Documentation]     Only for debug purposes
    [Arguments]    ${nodename}
    ${content}=    Get File    /tmp/logs/${nodename}.log
    Log  "[RabbitMQ] ${nodename} logs ========"  console=yes
    Log  ${content}  console=yes
    Log  "[RabbitMQ] ${nodename} logs end ===="  console=yes
