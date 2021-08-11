# Copyright 2021 Bloomberg Finance L.P.
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
Resource          libs/RabbitMQ.robot
Suite Setup       Run Keywords 
...               Smoke suite setup  AND
...               RabbitMQ suite setup
Suite Teardown    Terminate All Processes    kill=True

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


*** Test Cases ***
Smoke Test
    Log  ""  console=yes
    Make config directory
    Configure plugins
    Start rabbitmq process  pid_file=/tmp/shared1.pid
    ...                     nodename=shared1
    ...                     nodeport=5800
    ...                     configfile=${ROBOT_SOURCE_DIR}/integration/rabbitmq1
    Start rabbitmq process  pid_file=/tmp/shared2.pid
    ...                     nodename=shared2
    ...                     nodeport=5801
    ...                     configfile=${ROBOT_SOURCE_DIR}/integration/rabbitmq2
    Wait rabbitmq process  pid_file=/tmp/shared1.pid
    ...                    nodename=shared1
    Wait rabbitmq process  pid_file=/tmp/shared2.pid
    ...                    nodename=shared2
    Setting definitions  port=15800  file=${SMOKE_PATH}/full-definitions.json
    Setting definitions  port=15801  file=${SMOKE_PATH}/full-definitions.json
    Make admin user  shared1
    Make admin user  shared2

    Log  "Begin integration testing"
    ...  console=${LOG_CONSOLE}
    ${result} =  Run Process  uname
    ${uname}=  Set Variable  ${result.stdout}
    Log  ${uname}
    ...  console=${LOG_CONSOLE}
    ${result} =  Run Process  node
    ...                       ${SMOKE_PATH}/index.js
    ...                       ${BUILD_PATH}/amqpprox
    ...                       ${BUILD_PATH}/amqpprox_ctl
    ...                       ${WAIT_TIME}
    ...                       stdout=STDOUT
    ...                       stderr=STDOUT
    Should contain  ${result.stdout}  Test success.
