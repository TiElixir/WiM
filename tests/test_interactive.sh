#!/bin/sh
set -eu

test_dir=${TMPDIR:-/tmp}/wim-test-$$
mkdir "$test_dir"
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

file=$test_dir/delayed.txt

# Keep the terminal idle before each action. The editor must remain open while
# read(2) waits for input rather than treating an idle read as EOF.
{
    sleep 1
    printf 'iX'
    sleep 1
    printf '\033'
    sleep 1
    printf 'n'
    sleep 1
    printf 'q'
} | script -qec "./wim '$file'" "$test_dir/session.log" >/dev/null 2>&1

test -f "$file"
expected=$(printf 'X\n')
actual=$(cat "$file")
test "$actual" = "$expected"

printf '%s\n' 'interactive tests: ok'
