## amqpprox HTTP Auth service

### Introduction
Out of the box, amqpprox will perform no authentication of incoming connections - they only need to present a valid vhost. Using the `AUTH SERVICE ...` ctl command amqpprox can be configured to query an http service to auth inbound connections - a bit like [RabbitMQ's http auth backend](https://github.com/rabbitmq/rabbitmq-server/tree/master/deps/rabbitmq_auth_backend_http).
This document describes what information is sent to the service, and how the service should respond.

### Request
The service will receive POST request from amqpprox proxy with following fields in [protobuf format](./authrequest.proto)
- Vhost - Vhost name for connecting clients
- SASL auth data
    - Auth mechanism - Authentication mechanism field extracted from START-OK AMQP connection method, sent by client during handshake. E.g. PLAIN
    - Credentials - Response field extracted from START-OK AMQP connection method, sent by client during handshake. E.g. '\0user\0password'

### Response
The service should respond to amqpprox proxy with following fields in [protobuf format](./authresponse.proto)
- Auth Result - This will decide, whether to allow or deny clients. It is a enum type, takes ALLOW or DENY value only.
- Reason - Reason for the returned auth result. It is an optional field, but good to specify. The reason(reply-text) field is defined as short string in AMQP 0.9.1 protocol implementation. So it will be truncated before sending to the amqp clients, if the size of the string is more than 255 characters.
- SASL auth data - It is an optional field. In case of absence, the START-OK AMQP connection method, received from clients will be sent to the broker without any modification during handshake.
    - Broker mechanism - Authentication mechanism field for START-OK AMQP connection method. This will be injected into START-OK AMQP connection method to send to the broker during handshake. E.g. PLAIN
    - Credentials - Response field for START-OK AMQP connection method. This will also be injectd into START-OK AMQP connection method to send to the broker during handshake. E.g.'\0user\0password'

The external HTTP auth service is used to authenticate clients during the handshake before connecting to the destination RabbitMQ broker. The service receives SASL fields (mechanism and credentials) to authenticate and replies with Allow/Deny, and (optionally) with overridden SASL fields for the outbound connection to the broker.

The service is called once for each client connection, with a 30 seconds default timeout for each service request.
