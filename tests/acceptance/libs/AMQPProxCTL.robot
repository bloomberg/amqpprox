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
AMQPProxCTL BACKEND add
    [Arguments]    ${name}  ${datacenter}  ${host}  ${port}
    ${result}=  AMQPProxCTL send command  BACKEND  ADD
    ...                                   ${name}
    ...                                   ${datacenter}
    ...                                   ${host}
    ...                                   ${port}
    RETURN    ${result}

AMQPProxCTL BACKEND delete
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  BACKEND  DELETE
    ...                                   ${name}
    RETURN    ${result}

AMQPProxCTL BACKEND print
    ${result}=  AMQPProxCTL send command  BACKEND  PRINT
    RETURN    ${result}


AMQPProxCTL CONN
    ${result}=  AMQPProxCTL send command  CONN
    RETURN    ${result}


AMQPProxCTL DATACENTER set
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  DATACENTER  SET
    ...                                   ${name}
    RETURN    ${result}

AMQPProxCTL DATACENTER print
    ${result}=  AMQPProxCTL send command  DATACENTER  PRINT
    RETURN    ${result}


AMQPProxCTL EXIT
    ${result}=  AMQPProxCTL send command  EXIT
    RETURN    ${result}


AMQPProxCTL FARM add_dns
    [Arguments]    ${name}  ${dnsname}  ${port}
    ${result}=  AMQPProxCTL send command  FARM  ADD_DNS
    ...                                   ${name}
    ...                                   ${dnsname}
    ...                                   ${port}
    RETURN    ${result}

AMQPProxCTL FARM add_manual
    [Arguments]    ${name}  ${selector}  ${backend}
    ${result}=  AMQPProxCTL send command  FARM  ADD
    ...                                   ${name}
    ...                                   ${selector}
    ...                                   ${backend}
    RETURN    ${result}

AMQPProxCTL FARM partition
    [Arguments]    ${name}  ${policy}
    ${result}=  AMQPProxCTL send command  FARM  POLICY
    ...                                   ${name}
    ...                                   ${policy}
    RETURN    ${result}

AMQPProxCTL FARM delete
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  FARM  DELETE
    ...                                   ${name}
    RETURN    ${result}

AMQPProxCTL FARM print
    ${result}=  AMQPProxCTL send command  FARM  PRINT
    RETURN    ${result}


AMQPProxCTL HELP
    ${result}=  AMQPProxCTL send command  HELP
    RETURN    ${result}


AMQPProxCTL LISTEN start
    [Arguments]    ${port}
    ${result}=  AMQPProxCTL send command  LISTEN  START
    ...                                   ${port}
    RETURN    ${result}

AMQPProxCTL LISTEN stop
    ${result}=  AMQPProxCTL send command  LISTEN  STOP
    RETURN    ${result}


AMQPProxCTL LOG console
    [Arguments]    ${verbosity}
    ${result}=  AMQPProxCTL send command  LOG  CONSOLE
    ...                                   ${verbosity}
    RETURN    ${result}

AMQPProxCTL LOG file
    [Arguments]    ${verbosity}
    ${result}=  AMQPProxCTL send command  LOG  FILE
    ...                                   ${verbosity}
    RETURN    ${result}


AMQPProxCTL MAP backend
    [Arguments]    ${vhost}  ${backend}
    ${result}=  AMQPProxCTL send command  MAP  BACKEND
    ...                                   ${vhost}
    ...                                   ${backend}
    RETURN    ${result}

AMQPProxCTL MAP farm
    [Arguments]    ${vhost}  ${name}
    ${result}=  AMQPProxCTL send command  MAP  FARM
    ...                                   ${vhost}
    ...                                   ${name}
    RETURN    ${result}

AMQPProxCTL MAP unmap
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  MAP  UNMAP
    ...                                   ${vhost}
    RETURN    ${result}

AMQPProxCTL MAP default
    [Arguments]    ${farmName}
    ${result}=  AMQPProxCTL send command  MAP  DEFAULT
    ...                                   ${farmName}
    RETURN    ${result}

AMQPProxCTL MAP remove_default
    ${result}=  AMQPProxCTL send command  MAP  REMOVE_DEFAULT
    RETURN    ${result}

AMQPProxCTL MAP print
    ${result}=  AMQPProxCTL send command  MAP  PRINT
    RETURN    ${result}


AMQPProxCTL SESSION disconnect_graceful
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   DISCONNECT_GRACEFUL
    RETURN    ${result}

AMQPProxCTL SESSION pause
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   PAUSE
    RETURN    ${result}

AMQPProxCTL SESSION force_disconnect
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   FORCE_DISCONNECT
    RETURN    ${result}

AMQPProxCTL STAT listen
    [Documentation]  format = (json|human)
    ...              filter = (overall|vhost=foo|backend=bar|source=baz|
    ...                        all|process|bufferpool)
    [Arguments]    ${format}  ${filter}
    ${result}=  AMQPProxCTL send command  STAT LISTEN
    ...                                   ${format}
    ...                                   ${filter}
    RETURN    ${result}

AMQPProxCTL STAT stop send
    ${result}=  AMQPProxCTL send command STAT STOP SEND
    RETURN    ${result}

AMQPProxCTL STAT send
    [Arguments]    @{arguments}
    ${result}=  AMQPProxCTL send command STAT SEND
    ...                                  @{arguments}
    RETURN    ${result}

AMQPProxCTL VHOST pause
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  PAUSE
    ...                                   ${vhost}
    RETURN    ${result}

AMQPProxCTL VHOST unpause
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  UNPAUSE
    ...                                   ${vhost}
    RETURN    ${result}

AMQPProxCTL VHOST backend_disconnect
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  BACKEND_DISCONNECT
    ...                                   ${vhost}
    RETURN    ${result}

AMQPProxCTL VHOST force_disconnect
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  FORCE_DISCONNECT
    ...                                   ${vhost}
    RETURN    ${result}


AMQPProxCTL send command
    [Arguments]   @{arguments}
    ${SOURCE_PATH}=       Get Environment Variable   SOURCE_PATH
    ${AMQPPROX_BIN_DIR}=  Get Environment Variable   AMQPPROX_BIN_DIR
    ${result}=  Run Process  ${AMQPPROX_BIN_DIR}/amqpprox_ctl
    ...                      /tmp/amqpprox
    ...                      @{arguments}
    ...                      shell=yes
    RETURN    ${result}
