PlayChain Core
==============

* [Getting Started](#getting-started)
* [Using the API](#using-the-api)
* [License](#license)

PlayChain Core is the PlayChain blockchain implementation (based on BitShares) and command-line interface.
The GUI wallet is [PlayChain GUI](https://playchainwallet.page.link/start).
For management of own account (exporer database and history, sign transactions)
one can use command-line interface or GUI wallet

Getting Started
---------------

We recommend building on Ubuntu 16.04 LTS, and the build dependencies may be installed with:

    sudo apt-get update
    sudo apt-get install autoconf cmake git libboost-all-dev libssl-dev g++ libcurl4-openssl-dev

To build after all dependencies are installed:

    git clone https://bitbucket.org/total-games/playchain-core.git
    cd playchain-core
    git checkout <LATEST_RELEASE_TAG>
    git submodule update --init --recursive
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)

**NOTE:** Versions of [Boost](http://www.boost.org/) 1.57 through 1.69 are supported. Newer versions may work, but
have not been tested. If your system came pre-installed with a version of Boost that you do not wish to use, you may
manually build your preferred version and use it with PlayChain by specifying it on the CMake command line.

Example: ``cmake -DBOOST_ROOT=/path/to/boost .``

**NOTE:** PlayChain requires a 64-bit operating system to build, and will not build on a 32-bit OS.

**NOTE:** PlayChain now supports Ubuntu 18.04 LTS

**NOTE:** PlayChain now supports OpenSSL 1.1.0

**After Building**, the `witness_node` can be launched with:

    ./build/programs/playchaind/playchaind --plugins witness

The node will automatically create a data directory including a config file. It may take several hours to fully synchronize
the blockchain. After syncing, you can exit the node using Ctrl+C and setup the command-line wallet by editing
`playchain_node_data/config.ini` as follows:

    rpc-endpoint = 127.0.0.1:8090

**NOTE:** By default the witness node will start in reduced memory ram mode by using some of the commands detailed in [Memory reduction for nodes](https://github.com/bitshares/bitshares-core/wiki/Memory-reduction-for-nodes).
In order to run a full node with all the account history you need to remove `partial-operations` and `max-ops-per-account` from your config file.

After starting the witness node again, in a separate terminal you can run:

    ./build/programs/cli_wallet/cli_wallet

Set your inital password:

    >>> set_password <PASSWORD>
    >>> unlock <PASSWORD>

If you send private keys over this connection, `rpc-endpoint` should be bound to localhost for security.

Use `help` to see all available wallet commands.

Using the API
-------------

We provide several different API's.  Each API has its own ID.
When running `playchaind`, initially three API's are available:
API 0 (or `database`) provides read-only access to the database, API 1 (or `login`) is
used to login and gain access to additional, restricted API's
add API `playchain` provides read-only access to the playchain logic database

Here is an example using `wscat` package from `npm` for websockets:

    $ npm install -g wscat
    $ wscat -c ws://127.0.0.1:8090
    > {"id":1, "method":"call", "params":[0,"get_accounts",[["1.2.0"]]]}
    < {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

We can do the same thing using an HTTP client such as `curl` for API's which do not require login or other session state:

    $ curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_accounts", [["1.2.0"]]], "id": 1}' http://127.0.0.1:8090/rpc
    {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

    $ curl --data '{"jsonrpc": "2.0", "method": "call", "params": ["playchain", "list_all_rooms", ["", 10]], "id": 1}' http://127.0.0.1:8090/rpc | python -m json.tool

**NOTE:** `python -m json.tool` used for pretty formatting only

API 0 is accessible using regular JSON-RPC:

    $ curl --data '{"jsonrpc": "2.0", "method": "get_accounts", "params": [["1.2.0"]], "id": 1}' http://127.0.0.1:8090/rpc


Our `doxygen` documentation contains the most up-to-date information about API's. To create
doxygen files use `doxygen` utility in root folder:

    cd playchain-core
    doxygen

The `playchain-core/doxygen` folder with documentation will be created

 
License
-------
PlayChain Core is under the MIT license. See [LICENSE](https://github.com/bitshares/bitshares-core/blob/master/LICENSE.txt)
for more information.
