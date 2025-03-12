#!/bin/bash

export CODE="hyacinth"
export NAME="$CODE-$USER"

podman run --rm -it --events-backend=file --hostname=hyacinth --network host -v $PWD:/workspace:z $CODE
