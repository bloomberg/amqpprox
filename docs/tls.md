# TLS Support in `amqpprox`

`amqpprox` can be configured to use TLS for ingress (server) and egress (client) connections.
Some configuration options are shared between these two scenarios, and some differ. This document describes the available configuration parameters.

`boost::asio::ssl` is used to provide TLS support, which internally uses OpenSSL. The actual version of OpenSSL may depend on how `amqpprox` is built. A genuinely secure TLS/RabbitMQ setup requires careful consideration and execution. Please refer to the appropriate OpenSSL docs for the version used. A reference for using TLS with RabbitMQ is available here: https://www.rabbitmq.com/ssl.html


## TLS Configuration
All examples in this section are applicable for ingress and egress connections. `INGRESS` is used only to keep examples consistent.

### 1. `amqpprox` Certificates

For both ingress and egress connections, the `amqpprox` certificate can be specified using 

`amqpprox_ctl /tmp/amqpprox INGRESS CERT_CHAIN_FILE /path/to/cert.crt`

This file path is given to OpenSSL which expects it to be in PEM format.

### 2. Private Keys
The ingress/egress private keys can be configured using:

`amqpprox_ctl /tmp/amqpprox INGRESS KEY_FILE /path/to/private.key`
or
`amqpprox_ctl /tmp/amqpprox INGRESS RSA_KEY_FILE /path/to/private.key`

PEM format is expected for both key file types.

### 3. Certificate Verification

The level of certificate verification performed during the TLS handshake is configured using `VERIFY_MODE`

#### Enable peer verification
E.g. the following instructs `amqpprox` to verify the opposing party's certificate during the connection.
`amqpprox_ctl /tmp/amqpprox INGRESS VERIFY_MODE PEER`

#### Disable peer verification
`amqpprox_ctl /tmp/amqpprox INGRESS VERIFY_MODE NONE`

#### Require other party to present certificate
`amqpprox_ctl /tmp/amqpprox INGRESS VERIFY_MODE FAIL_IF_NO_PEER_CERT`

### 4. CA Certificates

The CA Certificate(s) used to verify the other party are configured using:

`amqpprox_ctl /tmp/amqpprox INGRESS CA_CERT_FILE /path/to/cacert.crt`

This file path is given to OpenSSL which expects it to be in PEM format.

## Ingress configuration

Inbound TLS sessions can be configured by enabling a 'secure' port to listen on:
`amqpprox_ctl /tmp/amqpprox START_SECURE 5671`

A few parameters are only relevant to server connections.

### DH params for Forward Secrecy
https://wiki.openssl.org/index.php/Diffie-Hellman_parameters

dh parameters can be configured using:

`amqpprox_ctl /tmp/amqpprox INGRESS TMP_DH_FILE /path/to/dhparams.pem`
This file must be in PEM format.

### Only request client certificate once
`amqpprox_ctl /tmp/amqpprox INGRESS VERIFY_MODE CLIENT_ONCE`

 > See SSL_CTX_set_verify documentation for more information e.g. https://www.openssl.org/docs/manmaster/man3/SSL_CTX_set_verify.html

## Egress configuration

A RabbitMQ broker (backend) can be flagged as requiring a TLS connection during the declaration:

`amqpprox_ctl /tmp/amqpprox BACKEND ADD server1 local 127.0.0.1 5671 TLS`

 > Note this can be combined with the Proxy Protocol header as follows

`amqpprox_ctl /tmp/amqpprox BACKEND ADD server1 local 127.0.0.1 5671 TLS SEND-PROXY`
