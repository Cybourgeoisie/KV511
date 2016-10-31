default: tar

tar:
	tar cvfz KV511.tgz \
		./server-front/*.cpp \
		./client-front/*.cpp \
		./libs/*.cpp \
		./server-front/*.hpp \
		./client-front/*.hpp \
		./libs/*.hpp \
		./libs/*.py \
		./server-front/Makefile \
		./client-front/Makefile

untar:
	tar 

# For Docker only:

# By default, stop everything, rebuild, and then run
#default: stop build run
#
# Use this to build all three docker containers
#build:
#	docker build -t kv511-client ./client-front/;
#	docker build -t kv511-server-front ./server-front/;
#	docker build -t kv511-server-back ./server-back/;
#
# Start all three docker containers
#run:
#	docker-compose up -d;
#
## Stop all three docker containers
#stop:
#	docker-compose down;
#
## Use these to attach to the VMs directly
#client:
#	docker exec -it kv511-client /bin/bash;
#
#front:
#	docker exec -it kv511-server-front /bin/bash;
#
#back:
#	docker exec -it kv511-server-back /bin/bash;
