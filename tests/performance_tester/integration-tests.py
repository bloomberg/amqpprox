#
# Copyright 2022 Bloomberg Finance L.P.
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
#

import pytest
import socket
from contextlib import closing
import subprocess
import os
from time import sleep

AMQPPROX_PORT = 5672
PERF_TEST_PORT = 5671


def check_socket(host, port):
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        if sock.connect_ex((host, port)) == 0:
            return True
        else:
            return False


@pytest.fixture(scope="module")
def amqpprox():
    amqpprox = os.environ.get("AMQPPROX_BIN_DIR")

    if not amqpprox:
        amqpprox = "amqpprox"
    else:
        amqpprox = amqpprox + "/amqpprox"

    instance = subprocess.Popen(
        [
            amqpprox,
            "--listenPort",
            str(AMQPPROX_PORT),
            "--destinationPort",
            str(PERF_TEST_PORT),
            "--destinationDNS",
            "localhost"
        ],
    )

    while not check_socket("localhost", AMQPPROX_PORT):
        sleep(0.5)

    yield f"amqp://localhost:{AMQPPROX_PORT}"

    instance.kill()


def run_perf_test_command(amqpprox_url, message_size, num_messages, max_threads, clients):
    port = 19305
    env = os.environ.copy()
    env["RUST_LOG"] = "info"

    return subprocess.run(
        [
            env.get("CARGO_PATH", "cargo"),
            "run",
            "--release",
            "--manifest-path",
            "tests/performance_tester/Cargo.toml",
            "--",
            "--address",
            amqpprox_url,
            "--listen-address",
            f"127.0.0.1:{PERF_TEST_PORT}",
            "--message-size",
            str(message_size),
            "--num-messages",
            str(num_messages),
            "--max-threads",
            str(max_threads),
            "--clients",
            str(clients)
        ],
        env=env,
    )


def test_100_clients(amqpprox):
    assert run_perf_test_command(amqpprox, message_size=100, num_messages=1, max_threads=10, clients=100).returncode == 0
