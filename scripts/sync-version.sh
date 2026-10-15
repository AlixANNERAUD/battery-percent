#!/bin/sh
set -eu

cd "$(dirname "$0")/.."

if [ "$#" -gt 1 ]; then
    echo "usage: $0 [VERSION]" >&2
    exit 2
fi

if [ "$#" -eq 1 ]; then
    version=$1
else
    version=$(awk '
        /^\[package\]/ { section = 1; next }
        /^\[/ { section = 0 }
        section && /^version[ \t]*=/ {
            sub(/^[^"]*"/, "")
            sub(/".*$/, "")
            print
            exit
        }
    ' Cargo.toml)
fi

if ! printf '%s\n' "$version" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$'; then
    echo "invalid version: '$version'" >&2
    exit 1
fi

cmake_version=${version%%[-+]*}

replace() {
    file=$1
    expression=$2
    expected=$3
    sed "$expression" "$file" > "$file.sync-version.tmp"
    if cmp -s "$file" "$file.sync-version.tmp"; then
        rm -f "$file.sync-version.tmp"
    else
        cat "$file.sync-version.tmp" > "$file"
        rm -f "$file.sync-version.tmp"
        echo "updated $file"
    fi
    if ! grep -Fq "$expected" "$file"; then
        echo "could not set the version in $file" >&2
        exit 1
    fi
}

replace CMakeLists.txt \
    "s/^\(project(battery_percent VERSION \)[^ )]*/\1$cmake_version/" \
    "project(battery_percent VERSION $cmake_version "
replace library.json \
    "s/^\(  \"version\": \"\)[^\"]*\"/\1$version\"/" \
    "  \"version\": \"$version\","
