#!/usr/bin/env bash
DOCKER_BUILDKIT=1 docker image build -t ac_test_package_ngransac:1.0 `pwd`
