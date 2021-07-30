# Configuring `amqpprox`

`amqpprox` is configured primarily through messages passed to it's [unix domain socket](https://en.wikipedia.org/wiki/Unix_domain_socket) (default `/tmp/amqpprox`).

`amqpprox_ctl` is a tool for conviently passing configuration commands over this socket.

```
------------------       ingress       -------------------       egress     -------------------
|     client      |      ------->      |       proxy      |     -------->   |   farm/backend  |
------------------                     -------------------                  -------------------
```         

## Config Commands

```
$ amqpprox_ctl /tmp/amqpprox HELP
AUTH (SERVICE hostname port target | ALWAYS_ALLOW | PRINT) - Change authentication mechanism for connecting clients
BACKEND (ADD name datacenter host port [SEND-PROXY] [TLS] | ADD_DNS name datacenter address port [SEND-PROXY] [TLS] | DELETE name | PRINT) - Change backend servers
CONN Print the connected sessions
DATACENTER SET name | PRINT
EXIT Exit the program gracefully.
FARM (ADD name selector backend* | PARTITION name policy | DELETE name | PRINT) - Change farms
HELP Print this help text.
LISTEN START port | START_SECURE port | STOP [port]
LOG CONSOLE verbosity | FILE verbosity
MAP (BACKEND vhost backend | FARM vhost name | UNMAP vhost | DEFAULT farmName | REMOVE_DEFAULT | PRINT) - Change mappings of resources to servers
MAPHOSTNAME DNS - Set up mapping of IPs to hostnames
SESSION  id# (PAUSE|DISCONNECT_GRACEFUL|FORCE_DISCONNECT) - Control a particular session
STAT (STOP SEND | SEND <host> <port> | (LISTEN (json|human) (overall|vhost=foo|backend=bar|source=baz|all|process|bufferpool)) - Output statistics
TLS (INGRESS | EGRESS) (KEY_FILE file | CERT_CHAIN_FILE file | RSA_KEY_FILE file | TMP_DH_FILE file | CA_CERT_FILE file | VERIFY_MODE mode* | CIPHERS (PRINT | SET ciphersuite(:ciphersuite)*))
```


## AUTH commands

The command represents authentication mechanism for connecting clients. By default all the clients will be allowed to connect to broker without any authentication. With the help of this command, one can set up an external HTTP auth service to authenticate clients, before allowing them to start communicating with broker. The schema structure for the HTTP auth service is defined [here](../authproto)

#### AUTH SERVICE hostname port target

Configures the external HTTP auth service to authenticate connecting clients.

For example: `AUTH SERVICE localhost 1234 /auth?tier=dev` will trigger queries to http://localhost:1234/auth?tier=dev when a client connects.

#### AUTH ALWAYS_ALLOW

Stops authentication for connecting clients. So all clients will be allowed to connect to broker.

#### AUTH PRINT

Prints information about current configured auth mechanism.

## BACKEND commands

A backend represents an instance of an AMQP broker.

`SEND-PROXY` tells the proxy to send a Proxy protocol header, necessary if broker is configured to require such header i.e. only accept connections from proxies.

`TLS` tells the proxy to use a TLS-enabled connection with the broker.

`datacenter` can be used for datacenter affinity partitioning - prioritizing backends that are in the same datacenter as the proxy.

#### BACKEND ADD name datacenter host port [SEND-PROXY] [TLS]

This adds a backend by `hostname` and `port`.

#### BACKEND ADD_DNS name datacenter address port [SEND-PROXY] [TLS]

This adds a backend by `address` and `port`.

#### BACKEND DELETE name

Deletes a backend by `name`. This does not affect existing connections to the deleted backend.

#### BACKEND PRINT

Prints the list of all configured backends in the format `name (datacenter): host ip:port`

## CONN commands

#### CONN

Prints the list of all current connections along with some metrics.

## DATACENTER commands

#### DATACENTER SET name

Sets the name of the datacenter where this instance of amqpprox is running. Useful for using datacenter affinity partition policy.


## EXIT commands

### EXIT

Gracefully shut down amqpprox.

## FARM commands

A farm represents a set of backends that a vhost can be mapped to. A client connecting to such vhost will be connected to one of the farm's backends chosen by the configured backend selector. 

#### FARM ADD name selector backend*

Adds a farm with `selector` backend selector. Accepts multiple backends by name as argument. Backends must be added beforehand using `BACKEND ADD` command.

#### FARM PARTITION name policy

Applies `policy` partition policy to the farm given by `name`. Partition policy allows backend selectors to prioritize backends. The policies are executed in the order they are applied, e.g. the output of the first partitioning policy is the input to the second policy. Applying a policy only affects new connections; existing connections remain with the same backend.

#### FARM DELETE name

Deletes farm by `name`. This does not affect existing connections to the backends of the deleted farm.

#### FARM PRINT

Prints the list of farms registered with the proxy.

## HELP commands

#### HELP

Prints command help text.

## LISTEN commands

#### LISTEN START port

Starts listening for ingress (client => proxy) connections on the given `port`.

#### LISTEN START_SECURE port

Starts listening for TLS-enabled ingress (client => proxy) connections on the given `port`. Use [TLS commands](#tls-commands) to configure beforehand.

#### LISTEN STOP port

Stops listening on the given `port`. This does not affect existing sessions.

## LOG commands

Verbosity argument is an integer 0..5 corresponding to `{"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"}`. For example, `amqpprox_ctl /tmp/amqpprox/control LOG CONSOLE 5` sets console logging verbosity to `TRACE`.

#### LOG CONSOLE verbosity

Sets console logging verbosity.

#### LOG FILE verbosity

Sets logfile logging verbosity.

## MAP commands

There are two types of vhost mapping available:
- backend mapping;
- farm mapping.

Backend mapping maps vhost clients to a particular backend i.e. an AMQP broker instance. 
Farm mapping maps vhost clients to a farm - a set of backends. Clients will be connected to one of farm's backends depending on configured partitioning policies and backend selector.


#### MAP BACKEND vhost backend

Maps `vhost` to `backend`. Clients connecting to the vhost will be connected to the specified backend.

#### MAP FARM vhost name

Maps `vhost` to farm given by `name`. Clients connecting to the vhost will be connected to one of the backends in the specified farm.

#### MAP UNMAP vhost

Unmaps `vhost`.

#### MAP DEFAULT farmName

Sets the farm given by `farmName` as the default farm. If a vhost is not explicitly mapped, the default farm will be used.

#### MAP REMOVE_DEFAULT

Unsets default farm.

#### MAP PRINT

Prints the list of mappings.

## MAPHOSTNAME commands

#### MAPHOSTNAME DNS

Enables `DNSHostnameMapper` to do reverse DNS lookups on client addresses. Resolved hostnames will be injected into client properties sent to the broker. If not enabled, client IP address will be used instead of hostname.

## SESSION

A session represents a client's connection to a broker via the proxy, consisting of ingress (client => proxy) and egress (proxy => broker) connections. Session commands take session id as argument. Session id's can be found by running `CONN`.

#### SESSION #id PAUSE

Pauses a session. Proxy will stop processing incoming data from ingress (client => proxy).

#### SESSION #id DISCONNECT_GRACEFUL

Gracefully disconnects a session by sending a synthetic Connection.Close method to the client.

#### SESSION #id FORCE_DISCONNECT

Forcefully disconnects a session by terminating both ingress and egress connections. 

## STAT commands
#### STAT SEND host port

Adds a `host:port` endpoint to send metrics to. This currently does not support filtering the metrics.

#### STAT STOP SEND

Stops sending metrics to all configured endpoints.

#### STAT LISTEN (json|human)

Streams metrics to stdout. Pass `json` or `human` to specify output format. Metrics can be filtered by passing `overall|vhost=foo|backend=bar|source=baz|all|process|bufferpool`.

## TLS commands

`TLS` command is used to configure TLS-enabled connections for both ingress (client => proxy) and egress (proxy => broker). See [TLS usage documentation](https://github.com/bloomberg/amqpprox/blob/main/docs/tls.md).

#### TLS (INGRESS | EGRESS) KEY_FILE file

Configures the private key to be used for ingress/egress connections.

#### TLS (INGRESS | EGRESS) CERT_CHAIN_FILE file

Configures the PEM format certificate to be sent to the other party.

#### TLS (INGRESS | EGRESS) RSA_KEY_FILE file

Configures the RSA private key to be used for ingress/egress connections.

#### TLS (INGRESS | EGRESS) TMP_DH_FILE file

Configures Diffie-Hellman parameters.

#### TLS (INGRESS | EGRESS) CA_CERT_FILE file

Configures the CA Certificate(s) used to verify the other party.

#### TLS (INGRESS | EGRESS) VERIFY_MODE mode*

Configures the peer verification mode. Possible modes are `PEER`, `NONE`, `FAIL_IF_NO_PEER_CERT`, `CLIENT_ONCE`.

#### TLS (INGRESS | EGRESS) CIPHERS PRINT

Prints configured allowed cipher set.

#### TLS (INGRESS | EGRESS) CIPHERS SET ciphersuite(:ciphersuite)*

Configures allowed cipher set.


## VHOST commands

#### VHOST PAUSE vhost

Pauses all sessions for the given `vhost`. New client connections are still accepted, but proxy will stop processing incoming data from ingress (client => proxy).

#### VHOST UNPAUSE vhost

Resumes all sessions for the given `vhost`.

#### VHOST BACKEND_DISCONNECT vhost

Closes egress (proxy => broker) sockets for all sessions for the given `vhost`.

#### VHOST FORCE_DISCONNECT vhost

Closes both ingress (client => proxy) and egress (proxy => broker) sockets for all sessions for the given `vhost`.

#### VHOST PRINT

Prints the list of vhosts and their paused/unpaused state.
