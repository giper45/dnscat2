FROM ruby:3.0

LABEL MAINTAINER="Mark Percival <m@mdp.im>"

EXPOSE 53/udp
COPY . /app
WORKDIR /app
ENV DNSCAT_SECRET=""
ENV DNS_OPTS=""

RUN gem install bundler && bundle install
RUN echo '#!/bin/sh' > run.sh && \
    echo 'ruby dnscat2.rb $( [ -n "$DNSCAT_SECRET" ] && echo --secret=$DNSCAT_SECRET ) $( [ -n "$DNS_OPTS" ] && echo --dns $DNS_OPTS )' >> run.sh && \
    chmod +x run.sh

ENTRYPOINT  tail -f /dev/null


# Run it
#   docker run -p 53:53/udp -it --rm mpercival/dnscat2 ruby ./dnscat2.rb foo.org
