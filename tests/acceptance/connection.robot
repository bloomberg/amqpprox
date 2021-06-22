# Copyright 2020 Bloomberg Finance L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

*** Settings ***
Library           OperatingSystem
Library           Process
Resource          libs/AMQPClient.robot
Resource          libs/RabbitMQ.robot
Resource          libs/AMQPProxCTL.robot
Resource          libs/AMQPProx.robot
Resource          libs/Consumer.robot
Suite Setup       Run Keywords
...               Smoke suite setup  AND
...               RabbitMQ suite setup
Suite Teardown    Terminate All Processes    kill=True
Test Setup        Connection Test Setup
Test Teardown     Connection Test Teardown
Test Timeout      10 minutes

*** Keywords ***
Smoke suite setup
    ${ROBOT_SOURCE_DIR}=  Get Environment Variable   ROBOT_SOURCE_DIR
    ${BINARY_PATH}=       Get Environment Variable   BINARY_PATH
    ${SOURCE_PATH}=       Get Environment Variable   SOURCE_PATH
    ${SMOKE_PATH}=        Get Environment Variable   SMOKE_PATH
    ${WAIT_TIME}=         Get Environment Variable   WAIT_TIME
    ${LOG_CONSOLE}=       Get Environment Variable   LOG_CONSOLE
    ${BUILD_PATH}=       Get Environment Variable   BUILD_PATH
    Set suite variable    ${ROBOT_SOURCE_DIR}
    Set suite variable    ${BINARY_PATH}
    Set suite variable    ${SOURCE_PATH}
    Set suite variable    ${SMOKE_PATH}
    Set suite variable    ${WAIT_TIME}
    Set suite variable    ${LOG_CONSOLE}
    Set suite variable    ${BUILD_PATH}

Connection Test Setup
    Log  ""  console=yes
    Make config directory
    Configure plugins
    Log  "Starting Rabbit"  console=yes
    ${RabbitMQProcess}=  Start rabbitmq process  pid_file=/tmp/shared1.pid
    ...                     nodename=shared1
    ...                     nodeport=5800
    ...                     configfile=${ROBOT_SOURCE_DIR}/integration/rabbitmq1
    Wait rabbitmq process  pid_file=/tmp/shared1.pid
    ...                    nodename=shared1
    Setting definitions  port=15800  file=${SMOKE_PATH}/full-definitions.json
    Make admin user  shared1

    Log  "Start AMQPProx and wait 5s"  console=yes
    ${AMQPProxProcess}=  AMQPProx start
    Sleep  5s

    Log  "Configure AMQPProx"  console=yes
    AMQPProxCTL log console  5
    AMQPProxCTL log file  5
    AMQPProxCTL backend add  shared1-1  dc-2  127.0.0.1  5800
    AMQPProxCTL farm add_manual  shared1  round-robin  shared1-1
    AMQPProxCTL map farm  /  shared1
    AMQPProxCTL map backend  foobar  shared1-1
    AMQPProxCTL datacenter set  dc-2
    AMQPProxCTL listen start  5555

    Log  "Waiting 5s"  console=yes
    Sleep  5s
    Set suite variable  ${AMQPProxProcess}
    Set suite variable  ${RabbitMQProcess}

Connection Test Teardown
    AMQPProx stop  ${AMQPProxProcess}
    RabbitMQ stop  ${RabbitMQProcess}
    Empty Directory  /tmp

*** Test Cases ***
Connection closed by client
    Log  "Starting AMQPClient"  console=yes
    Start AMQPClient  name=connection_close
    Sleep  4s

    Log  "Graceful Disconnect"  console=yes

    AMQPProxctl session disconnect_graceful  1
    Log  "Waiting 6s"  console=yes
    Sleep  6s

    AMQPClient has log  name=connection_close  content=on_open
    AMQPClient has log  name=connection_close  content=Exiting program
    RabbitMQ has log  nodename=shared1  content=accepting AMQP connection
    RabbitMQ has log  nodename=shared1  content=closing AMQP connection

Connection closed by proxy for AMQPClient
    Log  "Starting AMQPClient"  console=yes

    Start AMQPClient  name=connection
    Sleep  6s


    Log  "Graceful Disconnect"  console=yes
    AMQPProxctl session disconnect_graceful  1

    Log  "Waiting 6s"  console=yes
    Sleep  6s


    AMQPClient has log  name=connection  content=on_open
    AMQPClient has log  name=connection  content=ConnectionError: (200
    AMQPClient has log  name=connection  content=Exiting program
    RabbitMQ has log  nodename=shared1  content=accepting AMQP connection
    RabbitMQ has log  nodename=shared1  content=closing AMQP connection

Connection closed by proxy for RabbitMQ Consumer
    Log  "Starting RabbitMQ Consumer"  console=yes

    Start RabbitMQ Consumer  name=consumer1
    Sleep  6s

    Log  "Graceful Disconnect"  console=yes

    AMQPProxctl session disconnect_graceful  1

    Log  "Waiting 6s"  console=yes
    Sleep  6s

    RabbitMQ Consumer has log  name=consumer1
    ...                        content=Connection opened
    RabbitMQ Consumer has log  name=consumer1
    ...                        content=Connection closed
    RabbitMQ Consumer has log  name=consumer1
    ...                        content=Exiting program

    RabbitMQ has log  nodename=shared1  content=accepting AMQP connection
    RabbitMQ has log  nodename=shared1  content=closing AMQP connection
