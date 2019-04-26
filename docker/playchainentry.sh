#!/bin/bash

echo '>> $USER = '$USER
echo '>> $HOME = '$HOME
echo '>> $LIVE_TESTNET = '$LIVE_TESTNET
echo '>> $PLAYCHAIN_HOME = '$PLAYCHAIN_HOME
echo '>>'
echo '>> $NODE = '$NODE
echo '>> $PLAYCHAIN_SEED_NODES = '$PLAYCHAIN_SEED_NODES
echo '>> $PLAYCHAIN_RPC_ENDPOINT = '$PLAYCHAIN_RPC_ENDPOINT
echo '>> $PLAYCHAIN_PLUGINS = '$PLAYCHAIN_PLUGINS
echo '>> $PLAYCHAIN_REPLAY = '$PLAYCHAIN_REPLAY
echo '>> $PLAYCHAIN_RESYNC = '$PLAYCHAIN_RESYNC
echo '>> $PLAYCHAIN_P2P_ENDPOINT = '$PLAYCHAIN_P2P_ENDPOINT
echo '>> $PLAYCHAIN_WITNESS_ID = '$PLAYCHAIN_WITNESS_ID
echo '>> $PLAYCHAIN_PRIVATE_KEY = '$PLAYCHAIN_PRIVATE_KEY
echo '>> $PLAYCHAIN_TRACK_ACCOUNTS = '$PLAYCHAIN_TRACK_ACCOUNTS
echo '>> $PLAYCHAIN_PARTIAL_OPERATIONS = '$PLAYCHAIN_PARTIAL_OPERATIONS
echo '>> $PLAYCHAIN_MAX_OPS_PER_ACCOUNT = '$PLAYCHAIN_MAX_OPS_PER_ACCOUNT
echo '>> $PLAYCHAIN_ES_NODE_URL = '$PLAYCHAIN_ES_NODE_URL
echo '>> $PLAYCHAIN_TRUSTED_NODE = '$PLAYCHAIN_TRUSTED_NODE
echo '>> $PLAYCHAIN_ARGS = '$PLAYCHAIN_ARGS

## Supported Environmental Variables
#
#   * $PLAYCHAIN_SEED_NODES
#   * $PLAYCHAIN_RPC_ENDPOINT
#   * $PLAYCHAIN_PLUGINS
#   * $PLAYCHAIN_REPLAY
#   * $PLAYCHAIN_RESYNC
#   * $PLAYCHAIN_P2P_ENDPOINT
#   * $PLAYCHAIN_WITNESS_ID
#   * $PLAYCHAIN_PRIVATE_KEY
#   * $PLAYCHAIN_TRACK_ACCOUNTS
#   * $PLAYCHAIN_PARTIAL_OPERATIONS
#   * $PLAYCHAIN_MAX_OPS_PER_ACCOUNT
#   * $PLAYCHAIN_ES_NODE_URL
#   * $PLAYCHAIN_TRUSTED_NODE
#   * $PLAYCHAIN_ARGS
#

ARGS=""
# Translate environmental variables
if [[ ! -z "$PLAYCHAIN_SEED_NODES" ]]; then
    for NODE in $PLAYCHAIN_SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi
if [[ ! -z "$PLAYCHAIN_RPC_ENDPOINT" ]]; then
    ARGS+=" --rpc-endpoint=${PLAYCHAIN_RPC_ENDPOINT}"
fi

if [[ ! -z "$PLAYCHAIN_REPLAY" ]]; then
    ARGS+=" --replay-blockchain"
fi

if [[ ! -z "$PLAYCHAIN_RESYNC" ]]; then
    ARGS+=" --resync-blockchain"
fi

if [[ ! -z "$PLAYCHAIN_P2P_ENDPOINT" ]]; then
    ARGS+=" --p2p-endpoint=${PLAYCHAIN_P2P_ENDPOINT}"
fi

if [[ ! -z "$PLAYCHAIN_WITNESS_ID" ]]; then
    ARGS+=" --witness-id=$PLAYCHAIN_WITNESS_ID"
fi

if [[ ! -z "$PLAYCHAIN_PRIVATE_KEY" ]]; then
    ARGS+=" --private-key=$PLAYCHAIN_PRIVATE_KEY"
fi

if [[ ! -z "$PLAYCHAIN_TRACK_ACCOUNTS" ]]; then
    for ACCOUNT in $PLAYCHAIN_TRACK_ACCOUNTS ; do
        ARGS+=" --track-account=$ACCOUNT"
    done
fi

if [[ ! -z "$PLAYCHAIN_PARTIAL_OPERATIONS" ]]; then
    ARGS+=" --partial-operations=${PLAYCHAIN_PARTIAL_OPERATIONS}"
fi

if [[ ! -z "$PLAYCHAIN_MAX_OPS_PER_ACCOUNT" ]]; then
    ARGS+=" --max-ops-per-account=${PLAYCHAIN_MAX_OPS_PER_ACCOUNT}"
fi

if [[ ! -z "$PLAYCHAIN_ES_NODE_URL" ]]; then
    ARGS+=" --elasticsearch-node-url=${PLAYCHAIN_ES_NODE_URL}"
fi

if [[ ! -z "$PLAYCHAIN_TRUSTED_NODE" ]]; then
    ARGS+=" --trusted-node=${PLAYCHAIN_TRUSTED_NODE}"
fi

if [[ -z ${USER} ]]; then
    USER=playchain
fi

if [[ "$NODE" = "" ]]; then
    echo "NODE is not set. Using config.ini for witness node."
    NODE="witness"
fi

# Additional env variable for binary
CONF_TYPE=$(echo $NODE | tr [:upper:] [:lower:])

if [[ ! "$CONF_TYPE" =~ ^(witness|seed)$ ]]; then
    echo "$CONF_TYPE is not in the list"
    exit 1
fi

NODED="/usr/local/bin/playchaind"

if [ ! -f "${HOME}/config.ini" ]; then
    echo "Config ${HOME}/config.ini file not found. Using default for '$CONF_TYPE' config"
    if [[ $CONF_TYPE = "seed" ]]; then
        cp /etc/playchain/config.ini.seed "${HOME}/config.ini"
    else
        # witness is default configuration
        cp /etc/playchain/config.ini.witness "${HOME}/config.ini"
    fi

    if [ $LIVE_TESTNET = ON ]; then
        cat /etc/playchain/seeds.ini.testnet >> "${HOME}/config.ini" || exit 1
    else
        cat /etc/playchain/seeds.ini.mainnet >> "${HOME}/config.ini" || exit 1
    fi
fi

#
ulimit -c unlimited

# Plugins need to be provided in a space-separated list, which
# makes it necessary to write it like this
if [[ ! -z "$PLAYCHAIN_PLUGINS" ]]; then
    exec chpst -u ${USER} ${NODED} -x "${HOME}/config.ini" --data-dir "${HOME}" ${ARGS} ${PLAYCHAIN_ARGS} --plugins "${PLAYCHAIN_PLUGINS}" 2>&1
else
    exec chpst -u ${USER} ${NODED} -x "${HOME}/config.ini" --data-dir "${HOME}" ${ARGS} ${PLAYCHAIN_ARGS} 2>&1
fi
