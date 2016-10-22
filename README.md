# KV511
A key-value store implemented with a client-server model.

# Requirements:
- Docker (CLI preferred)
- Make

# Developing and Testing
Build and run the Docker containers, then you can use your preferred code editor to modify the program. The Docker containers link their internal volumes to the folders that contain our code, so we can make changes without having to rebuild and redeploy Docker - the changes take place instantly, and we can use g++ within the containers to build using the same configurations on both of our machines and the same configuration that will be used on production.

# Why Use Docker?
Docker provides a portable development and production environment, so that we can build the applications on our own machines and then deploy to AWS without fear or repurcussions of mis-configurations.

Once we're ready to start deploying on AWS, Amazon has automatic tools to let us build new versions of the Docker containers, push them directly to AWS, and then deploy on the spot.

# How to Use Docker
There is a Makefile that consolidates the most common commands - building the docker containers, running them, stopping them, and attaching to them so that we can develop and debug.

## Building
From the root directory, just run `make build`

## Running the VMs
`make run`

## Stopping the VMs
`make stop`

## Attaching to any of the three VMs
- `make client` - attach to the client
- `make front` - attach to the server front-end
- `make back` - attach to the server back-end (not used until P2)