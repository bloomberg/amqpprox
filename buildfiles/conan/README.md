# buildfiles: Conan Edition

This provides a set of buildfiles using the Conan system and Conan Docker base
images in order to build the dependencies of amqpprox. The primary dependencies
are boost, gtest and OpenSSL. These are listed in the `conanfile.txt`.

If the arguments to `conan install` need to be changed, a `OVERRIDE_CONANOPTS`
environment variable can be passed during the `make setup` bootstrap phase.
This is likely to be needed if building with different toolchains/profiles.

We default the Docker based build to a clang base image, but this is
configurable through the `BUILD_DOCKERFILE` environment variable.

## Pinned dependencies

To keep the supply chain reproducible (and to satisfy the OpenSSF Scorecard
`Pinned-Dependencies` check), the build tooling installed here is pinned:

- Base images in the `*.Dockerfile`s are pinned by digest.
- Python tooling is installed with `pip install --require-hashes` from
  `requirements.txt` (Conan, used by CI and the build images) and
  `integration-requirements.txt` (Conan plus the integration test tools).
- The Rust performance tester is built in a separate `rust` stage of
  `integration.Dockerfile` using `cargo build --locked`.

Dependabot (`.github/dependabot.yml`) raises monthly PRs to bump these pins.
To change a Python dependency by hand, edit the matching `.in` file and
regenerate the lock file with hashes:

```
uv pip compile --generate-hashes --universal --python-version 3.12 \
    requirements.in -o requirements.txt
uv pip compile --generate-hashes --universal --python-version 3.12 \
    integration-requirements.in -o integration-requirements.txt
```
