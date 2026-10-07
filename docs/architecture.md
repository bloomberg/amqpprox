# Code Architecture and Walkthrough

## Introduction

This document provides a quick walk-through guide to getting started in the
amqpprox codebase.

## Dependencies

amqpprox is designed to be lightweight in its build and runtime dependencies,
relying only on C++17, boost and cmake. For testing, Google Test and Google Mock
are required, but are downloaded by cmake in the standalone build.

Integration testing requires RabbitMQ, node and npm access, though it is most
readily achieved by running the integration process in a suitable Docker box.

## Layout

The code is laid out over several directories:

1. `libamqpprox` contains the bulk of the code. It is currently not designed
   to be utilized outside of the current executables that are generated, but
   is packaged as a library so as to allow this later if required by other
   binaries.
2. `amqpprox` contains the mainline for the proxy, and predominantly stitches
   together components from `libamqpprox`.
3. `amqpprox_ctl` contains a simple command-line interface to control a running proxy
   instance.
4. `tests` contains Google Test based unit tests, these should have no runtime
   dependencies.
5. `integration` contains integration testing against RabbitMQ instances.

## Concepts and Nomenclature

* **Resource:** The type of incoming client, ie usually determined by the
  declared vhost.
* **Session:** Is a logical connection through the proxy, connecting a Resource
  with an AMQP broker.
* **Backend:** A particular IP/port that corresponds to a potential target AMQP
  broker.
* **Farm:** A list of Backends which are to be load balanced between for
  serving establishing new sessions.

## Overall Architecture

The amqpprox architecture is that of a 'single-threaded' proxy, where each
connection traversing the proxy holds associated state in the client and
Backend programs. Asynchronous IO with boost ASIO is used throughout to enable
the program to serve many incoming clients at once without stalling or
blocking. The proxy currently maintains a 1-to-1 mapping between incoming
connections and outgoing connections, and as such the number of connections is
fundamentally limited by the number of ephemeral ports on the machine for
outgoing connections.

The proxy is set to listen on a particular port, and will then impersonate a
RabbitMQ server for the purposes of going through the handshake from a client.
Once the 'virtualhost' information is found at the end of the handshaking, the
proxy looks up by the virtualhost in its internal state which Farm to connect to
and selects a Backend to initiate a connection with. Once that connection is
established, it proceeds to handshake with the Backend RabbitMQ broker machine
with the same parameters the client initially sent it, augmented by information
in the client properties about the origin client's IP. Once the connection with
the Backend is fully established, the proxy then just moves AMQP frames from one
connection to the other in both directions.

Although it's primarily single threaded there are actually four threads in the
proxy right now:

1. **Server:** the primary thread, used for IO processing of AMQP traffic.
2. **Control:** for auxiliary processing, statistics gathering and handling of
   control messages.
3. **Logging to console:** handling IO for log messages destined for the
   console, this is internal to the logging framework.
4. **Logging to file:** handling IO for log messages destined for the log
   files, this is internal to the logging framework.

## Code Architecture

### Data Flow Walkthrough

The best place to start the walkthrough is the
[mainline](../amqpprox/amqpprox.m.cpp), which is holds references to most of the
actors in the proxy. It initializes the objects holding configuration state,
each of the [ControlCommands](../libamqpprox/amqpprox_controlcommand.h) and holds
all of the threads used in the system.

From here the [Server](../libamqpprox/amqpprox_server.cpp) component is
initialized and its event loop runs in the main thread. The `Server` is a boost
ASIO based server component, it starts listening on a given port and accepts
incoming connections. For each incoming connection it creates a
[Session](../libamqpprox/amqpprox_session.cpp)
[(h)](../libamqpprox/amqpprox_session.h) object, and stores it in a threadsafe
collection.

The `Session` component is also a boost ASIO based component, it holds
references to the sockets used for communication on the ingress (client to
proxy) and egress (proxy to broker) sides. The `Session` component is the
primary place where the read/write notifications come in from the sockets and
the egress connections are attempted. The `Session` component, upon receiving a
connection uses the [Connector](../libamqpprox/amqpprox_connector.h) component to
do the handshake with the client and get through to the point of knowing which
virtual host the connection is for. Once the virtual host is known the
[ConnectionSelector](../libamqpprox/amqpprox_connectionselector.h) is invoked to
determine where to make the egress connection. This is resolved using boost
ASIO, and the same `Connector` object is used to do the egress handshaking with
the broker. Once the `OpenOk` message has been passed to the connector the
`Session` is fully established and all future reads and writes are passed
through unchanged. The `Close` and `CloseOk` messages are looked for in order
to signify a graceful connection close down.

Internally to the `Session` when a read is received the buffer is passed to the
[PacketProcessor](../libamqpprox/amqpprox_packetprocessor.h) to split it into AMQP
frames, decode into AMQP methods and pass these to the `Connector` if required.
The `PacketProcessor` is also responsible for setting which slices of memory
are to be used by the `Session` for sending to ingress or egress sockets. The
decoding of frames and methods happen in the
[Frame](../libamqpprox/amqpprox_frame.h) and
[Method](../libamqpprox/amqpprox_method.h) components. The decoding into these
types only happens when the `Connector` is still negotiating the connection.
The actual methods all live in the `methods` namespace with the `methods_`
prefix on their filenames.

Most of `Session` does not need to be threadsafe on the main data path,
however, the [SessionState](../libamqpprox/amqpprox_sessionstate.h) does need to
be. It stores all the metrics and state of the session that is interogated from
the control thread and/or main thread.

### Buffer Handling

Most buffers used by `Session` for ingress and egress I/O come from a global
[BufferPool](../libamqpprox/amqpprox_bufferpool.h) that is owned by `main`. This
delegates different buffer sizes to individual
[BufferSource](../libamqpprox/amqpprox_buffersource.h) pools, or falls back to
the system allocator for larger allocations. This is so that we only use memory
for `Session` objects while the I/O is being processed or waiting for the
`write()` to the other socket to be completed. The pool gives out
[BufferHandle](../libamqpprox/amqpprox_bufferhandle.h) objects, which maintain
ownership of a buffer either from the pool or the free store. The
[Buffer](../libamqpprox/amqpprox_buffer.h) component does not convey any ownership
and is used as a slice of memory. The use of these Buffer is to avoid memory
allocations and buffer copying on the main path passing buffers through the
proxy.

### Buffer Bounds and the Exception Contract

`Buffer::copy` and `Buffer::skip` enforce their bounds at runtime rather than
with `assert`. Release builds define `NDEBUG`, so an assertion there would
provide no protection in a shipped binary; and because `available()` is
`d_length - d_offset` computed in `std::size_t`, an offset that moves past the
end does not fail loudly — it underflows to an enormous value, after which
every downstream `x > buffer.available()` guard silently passes. Those two
therefore throw rather than allow the offset to exceed the length.

`Buffer::seek` is the exception: it still only asserts, so it can put the
offset past the end in a release build and produce exactly that underflow. Its
callers pass values they already know to be in range — `Session::readBuffer`
passes a watermark — so this is a latent sharp edge rather than a live bug, but
it is worth knowing about before adding a new `seek` call site.

Two rules follow, and both matter when adding code:

1. **Throw `std::runtime_error`, not some other exception type.**
   [`Session::handleData`](../libamqpprox/amqpprox_session.cpp) catches
   `CloseError` and `std::runtime_error` specifically, and responds by
   disconnecting that one session. It does not catch `std::exception`. A bounds
   violation that throws `std::out_of_range`, `std::logic_error` or
   `std::bad_alloc` is *not* caught there — it unwinds into
   [`Server::run`](../libamqpprox/amqpprox_server.cpp), which closes the
   listening sockets and returns, stranding every live session on the proxy.
   The blast radius of the exception type is the difference between one dropped
   connection and a total outage.

2. **Decoders should prefer `tryCopy` to `copy`.** `Buffer::tryCopy<T>` reports
   truncation by return value and consumes nothing on failure, which lets a
   decoder reject a malformed frame as a normal `false` return. `Buffer::copy`'s
   throw is a backstop for a caller that checks neither `tryCopy` nor
   `available()` itself, not the intended error path.

### Field Table Nesting Depth

Field tables and field arrays nest, and the decoder walks them recursively:
`decodeFieldTable` -> `decodeFieldValue` -> `decodeFieldTable`. A nested table
costs six octets of input per level - a length prefix, an empty field name and
a type octet - so a single maximum-sized frame can demand roughly 25,000 levels
of recursion and exhaust the stack.

That failure is not containable. The exception contract above works because
`Session::handleData` can catch and drop one session, but stack exhaustion
raises no exception. So the decoder bounds nesting itself, at
`Constants::maxFieldTableNestingDepth`, and rejects anything deeper as a normal
`false` return. Real client properties nest one or two levels, so the limit is
far above legitimate traffic. The count is carried by private
`decodeField*Impl` overloads rather than by the public entry points, so no
caller can supply a depth of its own.

This bounds recursion **depth only**, and that is the whole of what it fixes.
It does not bound the *width* of a table: the cheapest possible field is two
octets - an empty name and a `'V'` no-value type - so a maximum-sized frame
still decodes into tens of thousands of `std::pair<std::string, FieldValue>`
entries, which is a substantial heap amplification from a small pre-auth input.
Bounding that would need a separate field-count limit and is not attempted
here.

Note that not every `Buffer` write path runs inside `handleData`'s try block.
`Session::handleSessionError`, the posted authentication callback, the
connection rate-limit timer and the TLS handshake callback all reach
`sendSyntheticData` from their own ASIO handlers. Any new bounds check on a
write path used by those callers needs its own handling, or it becomes a
proxy-wide failure rather than a session-level one.
