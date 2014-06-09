#!/bin/bash

URL="$1"
OUTPUT="/dev/null"

wget --output-document=$OUTPUT $1
