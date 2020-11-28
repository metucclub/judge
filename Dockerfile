FROM debian:buster-slim

ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    GOROOT=/usr/local/go \
    GOPATH=/root/go \
    PATH=$GOPATH/bin:$GOROOT/bin:/usr/local/cargo/bin:$PATH

RUN mkdir -p /usr/share/man/man1mkdir -p /usr/share/man/man1 && \
    apt-get update -y && \
    apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    gnupg2 \
    file \
    wget \
    curl \
    apt-transport-https \
    software-properties-common \
    dirmngr

RUN curl https://dmoj.ca/dmoj-apt.key | apt-key add - && \
    add-apt-repository 'deb https://apt.dmoj.ca/ buster main' && \
    curl https://dl-ssl.google.com/linux/linux_signing_key.pub | apt-key add - && \
    curl https://storage.googleapis.com/download.dartlang.org/linux/debian/dart_stable.list > /etc/apt/sources.list.d/dart_stable.list && \
    wget -qO - https://adoptopenjdk.jfrog.io/adoptopenjdk/api/gpg/key/public | apt-key add - && \
    add-apt-repository --yes https://adoptopenjdk.jfrog.io/adoptopenjdk/deb/

RUN apt-get update -y && \
    apt-get install -y --no-install-recommends \
    gcc \
    g++ \
    ghc \
    cabal-install \
    xz-utils \
    bzip2 \
    libncurses5 \
    libssl-dev \
    libc6-dev \
    libseccomp-dev \
    libexpat1 \
    libffi6 \
    libsqlite3-0 \
    python \
    python-dev \
    python-pip \
    python-setuptools \
    python3 \
    python3-dev \
    python3-pip \
    python3-setuptools \
    ruby \
    mono-complete \
    openjdk-11-jre \
    openjdk-11-jdk \
    adoptopenjdk-8-hotspot \
    gfortran \
    dart \
    php-common \
    php-cli \
    swi-prolog \
    racket \
    sbcl \
    nasm \
    v8dmoj \
    bash \
    git \
    unzip \
    zip \
    nano \
    vim

RUN cabal update && cabal install vector

RUN wget -q -O rustup.sh "https://sh.rustup.rs" && \
    chmod +x rustup.sh && \
    ./rustup.sh -y --no-modify-path --default-toolchain 1.43.1 && \
    rm rustup.sh && \
    chmod -R a+w $RUSTUP_HOME $CARGO_HOME

RUN wget -q -O go.tar.gz "https://dl.google.com/go/go1.15.5.linux-amd64.tar.gz" && \
    tar -xzf go.tar.gz -C /usr/local && \
    rm go.tar.gz

RUN wget -q -O pypy.tar.bz2 "https://github.com/squeaky-pl/portable-pypy/releases/download/pypy-7.2.0/pypy-7.2.0-linux_x86_64-portable.tar.bz2" && \
    tar -xjf pypy.tar.bz2 -C /usr/local && \
    mv /usr/local/pypy-7.2.0-linux_x86_64-portable /usr/local/pypy && \
    rm pypy.tar.bz2

RUN wget -q -O pypy3.tar.bz2 "https://github.com/squeaky-pl/portable-pypy/releases/download/pypy3.6-7.2.0/pypy3.6-7.2.0-linux_x86_64-portable.tar.bz2" && \
    tar -xjf pypy3.tar.bz2 -C /usr/local && \
    mv /usr/local/pypy3.6-7.2.0-linux_x86_64-portable /usr/local/pypy3 && \
    rm pypy3.tar.bz2

RUN wget -q -O kotlin.zip "https://github.com/JetBrains/kotlin/releases/download/v1.4.20/kotlin-compiler-1.4.20.zip" && \
    unzip kotlin.zip -d /usr/local && \
    rm kotlin.zip

RUN mkdir -p /problems && mkdir -p /rust && mkdir -p /judge

COPY rust /rust
WORKDIR /rust
RUN cargo build

WORKDIR /judge

COPY requirements.txt /judge/requirements.txt
RUN pip3 install -r requirements.txt

COPY . /judge

RUN env DMOJ_REDIST=1 python3 setup.py develop && rm -rf build/ && \
    rm -rf /rust && \
    rm -rf ~/.cache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
