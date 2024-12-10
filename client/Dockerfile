FROM ubuntu:22.04 as builder

RUN apt-get update && apt-get install -y build-essential
COPY . /build
WORKDIR /build
RUN make


FROM ubuntu:22.04 
COPY --from=builder /build/dnscat /app/dnscat
WORKDIR /app
ENV DNSCAT_SERVER=""
ENV DNSCAT_SECRET=""

RUN echo '#!/bin/sh' > run.sh && \
    echo '/app/dnscat --dns server=${DNSCAT_SERVER},port=53 --secret=${DNSCAT_SECRET}' >> run.sh && \
    chmod +x run.sh


ENTRYPOINT tail -f /dev/null