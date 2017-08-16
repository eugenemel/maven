#!/bin/sh
export GIT_VERSION=$(git describe --tags --always)
echo $GIT_VERSION
