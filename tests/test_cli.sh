#!/bin/sh
set -eu

output=$(./wim --help)
case "$output" in
    *"Usage:"*"[file]"*) : ;;
    *)
        printf '%s\n' "unexpected help output: $output" >&2
        exit 1
        ;;
esac

if ./wim one two >/dev/null 2>&1; then
    printf '%s\n' 'too many arguments unexpectedly succeeded' >&2
    exit 1
fi

printf '%s\n' 'cli tests: ok'
