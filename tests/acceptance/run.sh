#!/bin/sh

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

export BINARY_PATH=${ROBOT_BINARY_DIR:=/usr/bin}
export SOURCE_PATH=${SOURCE_PATH:=/source}
export BUILD_PATH=${BUILD_PATH:=/build}
export ACCEPTANCE_PATH=${ACCEPTANCE_PATH:=/source/tests/acceptance}
export SMOKE_PATH=${SMOKE_PATH:=/source/tests/acceptance/integration}
export WAIT_TIME=${WAIT_TIME:=30}
export ROBOT_SOURCE_DIR=${ROBOT_SOURCE_DIR:=.}
export LOG_CONSOLE=${LOG_CONSOLE:=yes}
export PYTHONPATH=/source/tests/acceptance/libs/

exec python3.8 -m robot.run --outputdir ./logs "$(dirname $0)/*.robot"
