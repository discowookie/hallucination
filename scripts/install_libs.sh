#!/bin/bash

# Get the location of this script.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HALLUCINATION_DIR="${DIR}/.."

# Build and install GLFW
cd ${HALLUCINATION_DIR}/third_party/glfw-3.0.4 && \
cmake -G 'Unix Makefiles' . && \
make -j 32 && \
sudo make install

# Build and install PortAudio
cd ${HALLUCINATION_DIR}/third_party/portaudio && \
./configure && \
make -j 32 && \
sudo make install

# Build and install aubio
cd ${HALLUCINATION_DIR}/third_party/aubio && \
./waf configure && \
./waf build && \
sudo ./waf install