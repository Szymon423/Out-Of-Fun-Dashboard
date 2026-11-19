#!/usr/bin/env bash
# Opens a shell in the Yocto build container.
set -eu
TOP="$HOME/yocto"
PROJECT="$HOME/Projects/Personal/39-Out-Of-Fun-Dashboard"
mkdir -p "$TOP/home"
exec podman run --rm -it \
  --userns=keep-id \
  --security-opt label=disable \
  -v "$TOP/home:$HOME" \
  -v "$TOP:$TOP" \
  -v "$PROJECT:$PROJECT" \
  -w "$PROJECT/01_Software/yocto" \
  --pull=never localhost/dashboard-yocto:wrynose bash