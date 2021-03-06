#!/usr/bin/env bash
# run tests in docker on travis ci
# reason:
# - build all things on ubuntu 14.04 is hard
# - even if build successfully, address sanitizer won't work on 14.04

# break after command error
set -e

# get absolute paths
PROJECT_DIR=$(realpath ../)

# start test container
Command=$(cat <<"EOF"
  apt-get update && \
  apt-get install -y ca-certificates software-properties-common && \
  add-apt-repository -y ppa:compiv/cpv-project
  add-apt-repository -y ppa:ubuntu-toolchain-r/test
  apt-get update
  apt-get install -y seastar libgtest-dev g++-9
  cd /project/tests && \
  sh run_tests.sh
EOF
)
docker run \
  -ti \
  --rm \
  --privileged \
  --net host \
  --volume "${PROJECT_DIR}:/project" \
  --sysctl net.core.somaxconn=65535 \
  ubuntu:18.04 \
  bash -c "$Command"

# cleanup
# docker rm -f $(docker ps -a -q)

