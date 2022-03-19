# it's used in a FROM so we need it early
ARG BASE

FROM goreleaser/nfpm:v2.18.1

ARG BASE
FROM $BASE

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update

ARG JPEG
RUN apt-get --no-install-recommends -y install cmake build-essential \
    libx11-dev libfreetype6-dev $JPEG-dev curl ca-certificates fakeroot \
    libxft2-dev

WORKDIR /workspace/
COPY ./ /workspace/

ENV BUILD_TYPE=Release

RUN set -e -x; \
  cmake -E make_directory build; \
  cd build; \
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DTESTS=ON ..; \
  DESTDIR=target cmake --build . --target install --config $BUILD_TYPE

ARG REF_NAME
ARG BASE
COPY --from=0 /usr/bin/nfpm /usr/local/bin
RUN set -e -x; \
  cd build; \
  sed "s/libjpeg62-turbo/${JPEG}/" ../nfpm.yaml.in > nfpm.yaml ; \
  export SEMVER="${REF_NAME#release-}~$(echo $BASE | sed 's/[^a-z0-9-]//g')"; \
  nfpm pkg \
    -f nfpm.yaml \
    -p deb \
    -t ../

FROM scratch
COPY --from=1 /workspace/*.deb .
