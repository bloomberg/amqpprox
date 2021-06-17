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
    [Return]    ${result}

AMQPProxCTL BACKEND delete
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  BACKEND  DELETE
    ...                                   ${name}
    [Return]    ${result}

AMQPProxCTL BACKEND print
    ${result}=  AMQPProxCTL send command  BACKEND  PRINT
    [Return]    ${result}


AMQPProxCTL CONN
    ${result}=  AMQPProxCTL send command  CONN
    [Return]    ${result}


AMQPProxCTL DATACENTER set
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  DATACENTER  SET
    ...                                   ${name}
    [Return]    ${result}

AMQPProxCTL DATACENTER print
    ${result}=  AMQPProxCTL send command  DATACENTER  PRINT
    [Return]    ${result}


AMQPProxCTL EXIT
    ${result}=  AMQPProxCTL send command  EXIT
    [Return]    ${result}


AMQPProxCTL FARM add_dns
    [Arguments]    ${name}  ${dnsname}  ${port}
    ${result}=  AMQPProxCTL send command  FARM  ADD_DNS
    ...                                   ${name}
    ...                                   ${dnsname}
    ...                                   ${port}
    [Return]    ${result}

AMQPProxCTL FARM add_manual
    [Arguments]    ${name}  ${selector}  ${backend}
    ${result}=  AMQPProxCTL send command  FARM  ADD
    ...                                   ${name}
    ...                                   ${selector}
    ...                                   ${backend}
    [Return]    ${result}

AMQPProxCTL FARM partition
    [Arguments]    ${name}  ${policy}
    ${result}=  AMQPProxCTL send command  FARM  POLICY
    ...                                   ${name}
    ...                                   ${policy}
    [Return]    ${result}

AMQPProxCTL FARM delete
    [Arguments]    ${name}
    ${result}=  AMQPProxCTL send command  FARM  DELETE
    ...                                   ${name}
    [Return]    ${result}

AMQPProxCTL FARM print
    ${result}=  AMQPProxCTL send command  FARM  PRINT
    [Return]    ${result}


AMQPProxCTL HELP
    ${result}=  AMQPProxCTL send command  HELP
    [Return]    ${result}


AMQPProxCTL LISTEN start
    [Arguments]    ${port}
    ${result}=  AMQPProxCTL send command  LISTEN  START
    ...                                   ${port}
    [Return]    ${result}

AMQPProxCTL LISTEN stop
    ${result}=  AMQPProxCTL send command  LISTEN  STOP
    [Return]    ${result}


AMQPProxCTL LOG console
    [Arguments]    ${verbosity}
    ${result}=  AMQPProxCTL send command  LOG  CONSOLE
    ...                                   ${verbosity}
    [Return]    ${result}

AMQPProxCTL LOG file
    [Arguments]    ${verbosity}
    ${result}=  AMQPProxCTL send command  LOG  FILE
    ...                                   ${verbosity}
    [Return]    ${result}


AMQPProxCTL MAP backend
    [Arguments]    ${vhost}  ${backend}
    ${result}=  AMQPProxCTL send command  MAP  BACKEND
    ...                                   ${vhost}
    ...                                   ${backend}
    [Return]    ${result}

AMQPProxCTL MAP farm
    [Arguments]    ${vhost}  ${name}
    ${result}=  AMQPProxCTL send command  MAP  FARM
    ...                                   ${vhost}
    ...                                   ${name}
    [Return]    ${result}

AMQPProxCTL MAP unmap
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  MAP  UNMAP
    ...                                   ${vhost}
    [Return]    ${result}

AMQPProxCTL MAP default
    [Arguments]    ${farmName}
    ${result}=  AMQPProxCTL send command  MAP  DEFAULT
    ...                                   ${farmName}
    [Return]    ${result}

AMQPProxCTL MAP remove_default
    ${result}=  AMQPProxCTL send command  MAP  REMOVE_DEFAULT
    [Return]    ${result}

AMQPProxCTL MAP print
    ${result}=  AMQPProxCTL send command  MAP  PRINT
    [Return]    ${result}


AMQPProxCTL SESSION disconnect_graceful
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   DISCONNECT_GRACEFUL
    [Return]    ${result}

AMQPProxCTL SESSION pause
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   PAUSE
    [Return]    ${result}

AMQPProxCTL SESSION force_disconnect
    [Arguments]    ${id}
    ${result}=  AMQPProxCTL send command  SESSION  ${id}
    ...                                   FORCE_DISCONNECT
    [Return]    ${result}

AMQPProxCTL STAT listen
    [Documentation]  format = (json|human)
    ...              filter = (overall|vhost=foo|backend=bar|source=baz|
    ...                        all|process|bufferpool)
    [Arguments]    ${format}  ${filter}
    ${result}=  AMQPProxCTL send command  STAT LISTEN
    ...                                   ${format}
    ...                                   ${filter}
    [Return]    ${result}

AMQPProxCTL STAT stop send
    ${result}=  AMQPProxCTL send command STAT STOP SEND
    [Return]    ${result}

AMQPProxCTL STAT send
    [Arguments]    @{arguments}
    ${result}=  AMQPProxCTL send command STAT SEND
    ...                                  @{arguments}
    [Return]    ${result}

AMQPProxCTL VHOST pause
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  PAUSE
    ...                                   ${vhost}
    [Return]    ${result}

AMQPProxCTL VHOST unpause
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  UNPAUSE
    ...                                   ${vhost}
    [Return]    ${result}

AMQPProxCTL VHOST backend_disconnect
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  BACKEND_DISCONNECT
    ...                                   ${vhost}
    [Return]    ${result}

AMQPProxCTL VHOST force_disconnect
    [Arguments]    ${vhost}
    ${result}=  AMQPProxCTL send command  VHOST  FORCE_DISCONNECT
    ...                                   ${vhost}
    [Return]    ${result}


AMQPProxCTL send command
    [Arguments]   @{arguments}
    ${SOURCE_PATH}=       Get Environment Variable   SOURCE_PATH
    ${BUILD_PATH}=       Get Environment Variable   BUILD_PATH
    ${result}=  Run Process  ${BUILD_PATH}/amqpprox_ctl/amqpprox_ctl
    ...                      /tmp/amqpprox
    ...                      @{arguments}
    ...                      shell=yes
    [Return]    ${result}
