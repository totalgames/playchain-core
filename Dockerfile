FROM phusion/baseimage:0.9.19
MAINTAINER Total Games LLC and contributors

ARG NODE
ARG USER=playchain
ARG PLAYCHAIN_HOME=/var/lib/playchain
ARG LIVE_TESTNET
ARG PORT_P2P=1777
ARG PORT_RPC=8090

ENV LIVE_TESTNET=${LIVE_TESTNET:-OFF}
ENV USER=${USER}
ENV NODE=${NODE}

ENV LANG=en_US.UTF-8
RUN \
    apt-get update -y && \
    apt-get install -y \
      g++ \
      autoconf \
      cmake \
      git \
      libbz2-dev \
      libreadline-dev \
      libboost-all-dev \
      libssl-dev \
      libncurses-dev \
      doxygen \
      libcurl4-openssl-dev \
    && \
    apt-get update -y && \
    apt-get install -y fish && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ADD . /playchain/src
WORKDIR /playchain/src

# Compile
RUN \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DGENESIS_TESTNET=${LIVE_TESTNET} \
        -DUTESTS_DISABLE_ALL_TESTS=ON \
        .. && \
    make -j$(nproc) && \
    make install && \
    #
    # Obtain version
    mkdir /etc/playchain && \
    git rev-parse --short HEAD > /etc/playchain/version && \
    cd / && \
    rm -rf /playchain/src

# Home directory $HOME
WORKDIR /
RUN useradd -s /bin/bash -m -d ${PLAYCHAIN_HOME} ${USER}
ENV HOME ${PLAYCHAIN_HOME}
RUN chown ${USER}:${USER} -R ${PLAYCHAIN_HOME}

# Volume
VOLUME ["/var/lib/playchain", "/etc/playchain"]

# rpc service:
EXPOSE ${PORT_RPC}
# p2p service:
EXPOSE ${PORT_P2P}

# default exec/config files
ADD docker/witness_config.ini /etc/playchain/config.ini.witness
ADD docker/seed_config.ini /etc/playchain/config.ini.seed
ADD docker/mainnet_seeds.ini /etc/playchain/seeds.ini.mainnet
ADD docker/testnet_seeds.ini /etc/playchain/seeds.ini.testnet
ADD docker/playchainentry.sh /usr/local/bin/playchainentry.sh
RUN chmod a+x /usr/local/bin/playchainentry.sh

# default execute entry
CMD ["/bin/bash", "/usr/local/bin/playchainentry.sh"]
