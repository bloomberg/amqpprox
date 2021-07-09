# amqpprox Performance Testing

`amqpprox` performance has been tested mainly in two ways:

1. Total data throughput achieved by connected clients (MB/s)
2. Total connection establishment throughput achieved (connections/s)

The performance tester in this folder helps with both of these by:

1. It contains a dummy AMQP 0.9.1. server which walks the AMQP handshake and ignores all future frames except close.
2. It can run parallel AMQP clients connecting to an amqpprox instance.
    1. Testing data throughput probably wants to send more, larger messages with fewer connections
    2. Testing connection throughput probably wants to send fewer, smaller messages with many connections.

## Usage Help

```
amqpprox_perf_tester

USAGE:
    amqpprox_perf_tester [OPTIONS] --listen-address <LISTEN_ADDRESS>

OPTIONS:
        --address <ADDRESS>
            [default: amqp://localhost:5672/]

        --clients <CLIENTS>
            Number of total AMQP clients to run [default: 10]

    -h, --help
            Print help information

        --listen-address <LISTEN_ADDRESS>
            IP Address/port for the dummy AMQP server to listen on

        --listen-cert <LISTEN_CERT>
            TLS cer used by the dummy AMQP server

        --listen-key <LISTEN_KEY>
            TLS key used by the dummy AMQP server. Must be the appropriate key for the provided cert

        --max-threads <MAX_THREADS>
            Max AMQP clients which can run in parallel [default: 50]

        --message-size <MESSAGE_SIZE>
            [default: 100]

        --num-messages <NUM_MESSAGES>
            [default: 10]

        --routing-key <ROUTING_KEY>
            Routing key passed for sent messages [default: routing-key]
```

## Performance Testing

### Setting up `amqpprox`

In order to use the perf tool we need to run amqpprox somewhere configured to point at the dummy AMQP server started by this tool.

`amqpprox` can either run on the same machine as `amqpprox_perf_tester`, or elsewhere. In this example the IP addresses for
these hosts are replaced with `<amqpprox host>` and `<perf tester host>`
```
$ amqpprox --listenPort 30672 --destinationPort 5672 --destinationDNS <perf tester host>
```

It's important to build `amqpprox_perf_tester` in release mode, especially for the TLS tests using `rustls`.

### Connection throughput testing

Run the perf tester with parameters which minimise the work required per connection, such as:
```
$ RUST_LOG=warn cargo run --release -- --address <amqpprox host>:30672 --listen-address 0.0.0.0:5672 --clients 100000 --max-threads 100 --message-size 1 --num-messages 1
100000 clients and 100000KB in 91.034013869seconds
1857.9424459252837 connections/second, 0.0018579424459252835 MB/second
```

These numbers were reached by tweaking until the connections/second figure stopped rising.
`amqpprox` showed ~70% CPU usage during the test run above. Since `amqpprox` is primarily
single threaded, this is a vague indication of it approaching saturation.

### Data throughput testing
Just like connection throughput testing but with paramters which exercise more, larger messages
rather than connections.

```
$ RUST_LOG=warn cargo run --release -- --address <amqpprox host>:30672 --listen-address 0.0.0.0:5672 --clients 10 --max-threads 10 --message-size 10000000 --num-messages 50
10 clients and 5000000000KB in 9.054987161seconds
1.1043637966788278 connections/second, 552.1818983394139 MB/second
```

### Testing with TLS

TLS impacts the performance of amqpprox by introducing extra overhead with each connection establishment & on-going overhead for data transferred.

This tool can be used to study part of this impact, by enabling TLS on the dummy AMQP server & configuring amqpprox
to connect to that dummy AMQP server over TLS.

#### Generate Certificates
To test TLS we need some keys/certificates for the connections.

In this setup we want to generate a key & certificate for the dummy AMQP server, then pass
this same certificate to amqpprox for validation.

```
openssl ecparam -out ec_key.pem -name secp256r1 -genkey
openssl req -new -key ec_key.pem -x509 -nodes -days 365 -out cert.pem
openssl pkcs8 -topk8 -nocrypt -in ec_key.pem -out key.pem # Convert EC Key to pkcs8
```

#### Start amqpprox
`amqpprox` doesn't have a quick start option for a TLS backend, so we need to manually start and configure it:

```
amqpprox &
amqpprox_ctl /tmp/amqpprox TLS EGRESS CA_CERT_FILE cert.pem
amqpprox_ctl /tmp/amqpprox TLS EGRESS VERIFY_MODE PEER

# Depending on your openssl version/settings you may need to specifically enable EC ciphers
amqpprox_ctl /tmp/amqpprox TLS EGRESS CIPHERS SET TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384

amqpprox_ctl /tmp/amqpprox BACKEND ADD backend dc <perf tester host> 5671 TLS
amqpprox_ctl /tmp/amqpprox FARM ADD farm round-robin backend
amqpprox_ctl /tmp/amqpprox MAP DEFAULT farm
amqpprox_ctl /tmp/amqpprox LISTEN START 30671
```

#### TLS Connection Throughput Testing

Running `amqpprox_perf_tester` with `--listen-cert <> --listen-key` tells the dummy AMQP server to listen for inbound TLS connections.

An example run, against an amqpprox instance configured as above:
```
RUST_LOG=warn cargo run --release -- --address amqp://<amqpprox host>:30672/ --listen-address 0.0.0.0:5671 --message-size 1 --num-messages 1 --clients 50000 --max-threads 300 --listen-cert cert.pem --listen-key key.pem
50000 clients and 50000KB in 50.811844034seconds
984.0225433767613 connections/second, 0.0009840225433767613 MB/second
```

#### TLS Data Throughput Testing


```
RUST_LOG=warn cargo run --release -- --address amqp://<amqpprox host>:30672/ --listen-address 0.0.0.0:5671 --clients 10 --max-threads 10 --message-size 10000000 --num-messages 100 --listen-cert cert.pem --listen-key key.pem
10 clients and 10000000000KB in 15.120871009seconds
0.6613375640892619 connections/second, 661.337564089262 MB/second
```
