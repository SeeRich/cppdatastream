default:
  @just --list

# Build the docker image
docker-build:
  @docker build -t datastream:latest -f docker/Dockerfile.2404 .

# Run the docker image
docker-run:
    docker run -it --rm -v $PWD:/usr/local/app datastream:latest

docker-clean:
    docker system prune -af