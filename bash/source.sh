#!/bin/bash

# Usage:
#   . source.sh

CUR_DIR=$(dirname ${BASH_SOURCE})

. ${CUR_DIR}/shell/common.sh
. ${CUR_DIR}/git/git-submodule.sh

unset CUR_DIR
