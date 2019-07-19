# Ubuntu image for compiling OpenTX NV14

# disco
FROM ubuntu:disco

# now under new "LABEL"
LABEL maintainer="derdoktor667@gmail.com"

# root needed for docker
USER root

# have a nice day
ENV DEBIAN_FRONTEND noninteractive

# /bin/sh points to Dash by default, reconfigure to use bash until Android
# build becomes POSIX compliant
RUN echo "dash dash/sh boolean false" | debconf-set-selections && dpkg-reconfigure -p critical dash

# soon this will be more "dockerish"
RUN apt-get -y update && apt-get -y dist-upgrade && apt-get -y install apt-utils build-essential cmake gcc git lib32ncurses6 libfox-1.6-dev gcc-arm-none-eabi python3 python-pil googletest googletest-tools && apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && apt-get autoclean -y && apt-get autoremove -y

# repo tool
ADD https://commondatastorage.googleapis.com/git-repo-downloads/repo /usr/local/bin/
RUN chmod 755 /usr/local/bin/*

# Set the working directory
WORKDIR /nv14-build

# Declare the mount point
VOLUME /nv14-build

ARG OPENTX_VERSION_SUFFIX=
ENV OPENTX_VERSION_SUFFIX ${OPENTX_VERSION_SUFFIX}
ENV PATH $PATH:/nv14-build/radio/util
ENV PATH $PATH:/nv14-build/tools

# build by script
ENTRYPOINT ["bash", "-c", "python3 /nv14-build/tools/utilize_docker.py $CMAKE_FLAGS"]
