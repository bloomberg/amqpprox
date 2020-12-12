#!/bin/sh

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
