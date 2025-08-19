# Known Development Issues:

### MAC OS Docker Filesystem Issue:
* There was/is an issue where random errors were occurring in the docker conan build.
* Related issues:
    * https://github.com/milvus-io/milvus/issues/35329
    * https://github.com/docker/for-mac/issues/7386
* Workaround:
    * Change the Virtual Machine Manager (VMM) used by Docker Desktop
    * Settings -> General -> Virtual Machine Options -> Apple Virtualization Framework
    * Docker VVM doesn\'t seem to work.