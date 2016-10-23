FROM gcc:4.9

RUN apt-get update -y && \
	apt-get install -y apt-utils coreutils vim gdb \
	make htop man nmap net-tools

RUN echo 'alias ll="ls -la"' >> ~/.bashrc

EXPOSE 54321

VOLUME /root/libs
VOLUME /root/client-front
WORKDIR /root/client-front

CMD /bin/bash
