# Sessions

## Overview

A session is a representation of the relationship between an incoming
connection from a client and the associated outgoing connection to the target
broker.

## Connection Management

### Incoming Connections

Clients connect to the proxy and negotiate with it as if it were a RabbitMQ
broker. A connected client is represented by the proxy as an ongoing `Session`.

### Outgoing Connections

When an incoming connection has been accepted by the proxy, traffic from that
client must be proxied to a real instance of a RabbitMQ broker. The precise
machine that the connection is opened with is determined by a number of 
mappings and configuration settings that are applied to produce a final result.

#### Backends

An individual proxy instance supports many possible outgoing brokers. These
brokers are added to the proxy as `Backend` instances.

A backend instance consists of the following data:

* A name
* A host
* A port
* A datacenter
* An IP
* Handshake options: TLS on/off, Proxy Protocol on/off

#### Farms

If several machines are in the same RabbitMQ cluster, they should be considered
for most purposes as equivalent to a single high-availability backend. To
achieve this, multiple backends can be combined together into a `Farm`.

In order to select a specific member of the farm for outgoing connections, it
must be configured with a `BackendSelector` and zero or more `PartitionPolicy`
instances.

A farm instance therefore consists of the following data:

* Zero or more backends
* Zero or more partition policies
* A backend selector

#### Partition Policies

Although a farm contains multiple backends, it is often the case that these are
not equally useful for a given client. A client will often want to prefer a
backend in a nearby physical location, or backends that are considered healthy.

To support this, a farm stores its backends internally in a `BackendSet` that
partitions the available backends into priority-ordered groups. That is, items
in the first partition will all be equally preferred to all items in the second
partition, which will all be preferred over those in the third, and so on.

By default, all backends in the farm will be stored in a single partition. 
However, partition policies can be applied to a farm to split the backends into
more suitable partitions. The specified policies are applied to the farm in the
order they are specified. Each policy will create new partitions as required
and move backends forwards or backwards as specified in their contract.

The following partition policies are supported:

* **datacenter-affinity**: Backends with the same datacenter property as the
  proxy instance will be partitioned higher than those with a different value.

Is it possible to create custom partition policies by implementing the
`PartitionPolicy` interface. See `AffinityPartitionPolicy` for an example.
Policies are designed to be compoundable, i.e. the output partitions of one policy can be input into the next, they are applied in series. 

#### Backend Selectors

When starting a connection, a specific backend must be chosen from the 
underlying backend set. This selector **must** adhere to the following contract
for a given session:

* If there is at least one backend that has not been attempted, a valid backend
  instance must be selected.
* A given backend must never be selected multiple times by distinct retry
  counts.
* With an incrementing retry count starting at 0, all backends in a 
  higher-priority partition must be attempted before any backend in a 
  lower-priority partition is attempted.

To facilitate this, a backend selector is given an immutable copy of the
backend set, an immutable copy of the `Marker`s for the backend set and a retry
count. When a connection is attempted on a given partition, the selector must
'mark' the partition.

The following backend selectors are supported:

* **round-robin**: The backend from the top-level partition that has least
  recently had a connection attempt will be attempted first. All other backends
  in the partition will be attempted, followed by the backend from the second-
  partition that has least recently had a connection attempt, and so on for all
  partitions.

Is it possible to create custom backend selectors by implementing the
`BackendSelector` interface. The implementation must adhere to the requirements
described above. See `RobinBackendSelector` for an example.
