#!/usr/bin/env bash
# Compute the next usdl2 release version.
#
#   With existing vX.Y.Z tags — highest tag + 1 patch (e.g. v0.1.0 → 0.1.1)
#   With no tags — version from setup.py (default first release)
#
# Usage:
#   ./scripts/next_release_version.sh
#   ./scripts/next_release_version.sh --verbose

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_REPO="$(cd "$SCRIPT_DIR/.." && pwd)"

VERBOSE=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --verbose | -v)
            VERBOSE=1
            shift
            ;;
        --help | -h)
            sed -n '2,12p' "$0" | sed 's/^# \?//'
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

read_setup_version() {
    local version=""
    if [[ ! -f "$SOURCE_REPO/setup.py" ]]; then
        echo "Error: setup.py not found in $SOURCE_REPO" >&2
        return 1
    fi
    version="$(grep -E '^\s*version\s*=' "$SOURCE_REPO/setup.py" | head -1 | sed -E 's/.*"([^"]+)".*/\1/')"
    if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "Error: could not read semver from setup.py (got: ${version:-<empty>})" >&2
        return 1
    fi
    echo "$version"
}

increment_patch() {
    local version="${1#v}"
    local major minor patch
    IFS=. read -r major minor patch <<< "$version"
    echo "${major}.${minor}.$((patch + 1))"
}

highest_tag_version() {
    local tag=""
    cd "$SOURCE_REPO"
    tag="$(git tag -l 'v[0-9]*.[0-9]*.[0-9]*' | sort -V | tail -1)"
    if [[ -z "$tag" ]]; then
        return 1
    fi
    echo "${tag#v}"
}

cd "$SOURCE_REPO"

if LAST_VERSION="$(highest_tag_version)"; then
    VERSION="$(increment_patch "$LAST_VERSION")"
    VERSION_SOURCE="git tag v${LAST_VERSION} + 1 patch"
else
    VERSION="$(read_setup_version)"
    VERSION_SOURCE="setup.py (no release tags yet)"
fi

if [[ "$VERBOSE" -eq 1 ]]; then
    echo "Version source: ${VERSION_SOURCE}"
    if [[ -n "${LAST_VERSION:-}" ]]; then
        echo "Last release tag: v${LAST_VERSION}"
    else
        echo "Last release tag: (none)"
    fi
    echo "Next version: ${VERSION}"
else
    echo "$VERSION"
fi
