#!/bin/bash

set -o errexit
set -o pipefail

if [ -n "$1" ]; then
  BRANCH="$1"
elif [ -n "$GITHUB_BASE_REF" ]; then
  BRANCH="