#!/bin/sh
set -e

git config --global --add safe.directory '*' 2>/dev/null || true

# pip installs cmake under /usr/local; some minimal PATHs miss it.
PATH="/usr/local/bin:${PATH}"
export PATH

if [ -z "${HOME:-}" ] || [ ! -w "${HOME:-/nonexistent}" ]; then
    HOME=/tmp
    export HOME
fi

exec "$@"
