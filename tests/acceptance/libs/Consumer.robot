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

