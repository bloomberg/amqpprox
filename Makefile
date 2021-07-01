UNAME_OVERRIDE ?= `uname`
BUILD_PARALLELISM ?= `nproc`
BUILDDIR ?= build/$(UNAME_OVERRIDE)
CUR_DIR ?= $(shell pwd)

test:
	cd $(BUILDDIR) && make -j$(BUILD_PARALLELISM)
	cd $(BUILDDIR) && make test CTEST_OUTPUT_ON_FAILURE=1

build:
	cd $(BUILDDIR) && make -j$(BUILD_PARALLELISM)

setup: BUILD_FLAVOUR ?= conan
setup:
	mkdir -p $(BUILDDIR)
	/bin/echo -n "$(BUILD_FLAVOUR)" > $(BUILDDIR)/flavour
	(test -x ./buildfiles/$(BUILD_FLAVOUR)/bootstrap && ./buildfiles/$(BUILD_FLAVOUR)/bootstrap $(BUILDDIR) $(CUR_DIR)) || true

init:
	cd $(BUILDDIR) && cmake $(EXTRA_CMAKE_ARGS) $(CUR_DIR)

clean:
	cd $(BUILDDIR) && make clean

DOCKER_IMAGE ?= amqpprox
DOCKER_BUILDDIR ?= build/docker-$(DOCKER_IMAGE)
DOCKER_ARGS ?= $(DOCKER_EXTRA_ARGS) -v $(CUR_DIR):/source -v $(CUR_DIR)/$(DOCKER_BUILDDIR):/build -it $(DOCKER_IMAGE)
DOCKER_RUN ?= docker run $(DOCKER_ARGS)

docker-setup: BUILD_FLAVOUR ?= conan
docker-setup: BUILD_DOCKERFILE ?= buildfiles/$(BUILD_FLAVOUR)/Dockerfile
docker-setup:
	mkdir -p $(DOCKER_BUILDDIR)
	docker build --target amqpprox_build_environment -t $(DOCKER_IMAGE) -f $(BUILD_DOCKERFILE) .
	$(DOCKER_RUN) bash -c 'make setup'

docker-init:
	$(DOCKER_RUN) bash -c "EXTRA_CMAKE_ARGS=$(EXTRA_CMAKE_ARGS) make init"

docker-build:
	$(DOCKER_RUN) bash -c 'make'

docker-clean:
	$(DOCKER_RUN) bash -c 'make clean'

docker-shell:
	docker run --privileged $(DOCKER_ARGS) bash

docs:
	doxygen Doxygen.config

.PHONY: test build setup init all clean docker-setup docker-init docker-build docker-shell docs
