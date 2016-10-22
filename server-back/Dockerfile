FROM gcc:4.9

RUN apt-get update -y && \
	apt-get install -y apt-utils coreutils vim gdb make

RUN echo 'alias ll="ls -la"' >> ~/.bashrc

VOLUME /root/kv511-server-back
WORKDIR /root/kv511-server-back

CMD /bin/bash
