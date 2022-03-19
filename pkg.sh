#!/bin/sh

# pkg.sh can be run to manually perform package builds using the same
# mechanisms used for automated builds during release

set -e

build() {
  DOCKER_BUILDKIT=1 \
  docker build \
    --build-arg BASE="$1" \
    --build-arg JPEG="$2" \
    --build-arg REF_NAME=$(git rev-parse --short HEAD) \
    --output type=local,dest=. \
    -f .github/workflows/Dockerfile \
    .
}

build ubuntu:18.04 libjpeg62
build ubuntu:20.04 libjpeg62
build ubuntu:22.04 libjpeg62
build debian:11    libjpeg62-turbo
build debian:sid   libjpeg62-turbo
