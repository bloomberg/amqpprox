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
