# Docker Container

This repository comes with built-in Dockerfile to support docker
containers. This README serves as documentation.

## Dockerfile Specifications

The `Dockerfile` performs the following steps:

1. Obtain base image (phusion/baseimage:0.10.1)
2. Install required dependencies using `apt-get`
3. Add playchain-core source code into container
4. Update git submodules
5. Perform `cmake` with build type `Release`
6. Run `make` and `make_install` (this will install binaries into `/usr/local/bin`
7. Purge source code off the container
8. Add a local playchain user and set `$HOME` to `/var/lib/playchain`
9. Make `/var/lib/playchain` and `/etc/playchain` a docker *volume*
10. Expose ports `8090` and `1777`
11. Add default config(s) from `docker/config.ini.*` and entry point script
12. Run entry point script by default

The entry point simplifies the use of parameters for the `witness node`
(which is run by default when spinning up the container) or for the `seed node`.

### Supported Environmental Variables

* `$PLAYCHAIN_SEED_NODES`
* `$PLAYCHAIN_RPC_ENDPOINT`
* `$PLAYCHAIN_PLUGINS`
* `$PLAYCHAIN_REPLAY`
* `$PLAYCHAIN_RESYNC`
* `$PLAYCHAIN_P2P_ENDPOINT`
* `$PLAYCHAIN_WITNESS_ID`
* `$PLAYCHAIN_PRIVATE_KEY`
* `$PLAYCHAIN_TRACK_ACCOUNTS`
* `$PLAYCHAIN_PARTIAL_OPERATIONS`
* `$PLAYCHAIN_MAX_OPS_PER_ACCOUNT`
* `$PLAYCHAIN_ES_NODE_URL`
* `$PLAYCHAIN_TRUSTED_NODE`

### Default config

The default configuration is:

    rpc-endpoint = 0.0.0.0:8090
    bucket-size = [60,300,900,1800,3600,14400,86400]
    history-per-size = 1000
    max-ops-per-account = 1000
    partial-operations = true

# Docker Hub

// TODO


