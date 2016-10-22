# Using Docker

# Ways to debug:
	docker logs [container]

# Development Builds
	# At /client/
	docker build -t kv511-client ./client/

	# At /server-front/
	docker build -t kv511-server-front ./server-front/

	# At /server-back/
	docker build -t kv511-server-back ./server-back/


# Development Run
# (A) Using Docker Compose
	
	# Bring up the client & server
	docker-compose up -d

	# Take down the client & server
	docker-compose down


# Pushing Production Docker Container
	sudo aws ecr get-login
	
	# Rest of the directions to come when EC2 is set up
