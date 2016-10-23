FROM gcc:4.9

RUN apt-get update -y && \
	apt-get install -y apt-utils coreutils vim gdb make htop

RUN echo 'alias ll="ls -la"' >> ~/.bashrc

VOLUME /root/libs
VOLUME /root/server-front
WORKDIR /root/server-front

CMD /bin/bash
