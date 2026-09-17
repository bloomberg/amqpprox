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

""" An executable module which automatically establishes connection with
    an amqpprox on localhost:5555. The virtual host to connect to can be
    passed as the first argument, and defaults to /.
"""

import logging
import sys
from amqp import Connection

LOG_FORMAT = ('%(levelname) -10s %(asctime)s %(name) -30s %(funcName)'
              '-35s %(lineno) -5d: %(message)s')
LOGGER = logging.getLogger("AMQPClientConnection")


class AMQPClientConnection(object):
    """ Class responsible for establishing AMQP connection and dumping
        everything received into the logger.
    """
    def __init__(self, host, virtual_host):
        LOGGER.info("creating connection for vhost {}".format(virtual_host))
        self.connection = Connection(host=host,
                                     virtual_host=virtual_host,
                                     heartbeat=4,
                                     on_open=self.on_open,
                                     on_blocked=self.on_blocked,
                                     on_unblocked=self.on_unblocked,
                                     on_tune_ok=self.on_tune_ok)

    def run(self):
        """Responsible for establishing a connection"""

        LOGGER.info("calling connect")
        try:
            # An exception is expected to raise when receiving close
            # from server, either during the handshake, for example when
            # the vhost has no mapping, or once connected
            self.connection.connect()
            LOGGER.info("connect called")
            while self.connection.connected:
                LOGGER.info("{}".format(self.connection.blocking_read()))
        except Exception as error:
            LOGGER.warning("{0}: {1!r}".format(type(error).__name__, error.args))

    def on_open(self, data):
        LOGGER.info(data)
        LOGGER.info("on_open")

    def on_blocked(self, reason):
        LOGGER.info("on_blocked for {}".format(reason))

    def on_unblocked(self):
        LOGGER.info("on_unblocked")

    def on_tune_ok(self):
        LOGGER.info("on_tune_ok")


def main():
    """ Main entry point """

    logging.basicConfig(level=logging.DEBUG, format=LOG_FORMAT)
    virtual_host = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1] else '/'
    example = AMQPClientConnection('localhost:5555', virtual_host)
    try:
        example.run()
    except KeyboardInterrupt:
        pass
    LOGGER.warn('Exiting program')


if __name__ == '__main__':
    main()
