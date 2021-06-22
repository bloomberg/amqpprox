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

