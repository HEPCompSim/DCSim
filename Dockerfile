ARG IMAGE_REGISTRY=mhorzela
ARG IMAGE_TAG=latest

FROM ${IMAGE_REGISTRY}/dcsim:${IMAGE_TAG} AS builder

# set user's environment variable
ENV CXX="g++" CC="gcc"
ENV LD_LIBRARY_PATH=/home/dcsim/.local/lib:/usr/lib/x86_64-linux-gnu:/usr/lib:/usr/local/lib64:/usr/local/lib

USER dcsim
WORKDIR /home/dcsim

# Copy the rest of the repository
COPY --chown=dcsim:dcsim . .

# Compile and install DCSim
RUN mkdir -p build && pushd build && \
    cmake .. && make -j${NCORES} && popd

USER root
RUN pushd build && make install && popd && ldconfig
USER dcsim

# Final image
FROM ${IMAGE_REGISTRY}/dcsim:${IMAGE_TAG}

# Copy built artifacts
COPY --from=builder /usr/local/bin/dc-sim /usr/local/bin/dc-sim
COPY --from=builder /usr/local/lib/libDCSim.so /usr/local/lib/libDCSim.so
COPY --from=builder /home/dcsim/.local /home/dcsim/.local
COPY --chown=dcsim:dcsim data/ /home/DCSim/data/
COPY --chown=dcsim:dcsim tools/ /home/DCSim/tools/

RUN dc-sim --help
