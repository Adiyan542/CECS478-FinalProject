# Use the official Ubuntu base image
# (Works natively on M2 Macs via ARM64 support)
FROM ubuntu:latest

# Avoid user interaction prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install Build Essentials (gcc, make), tcpdump, and basic net tools
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    tcpdump \
    iputils-ping \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# By default, keep the container running so we can exec into it
CMD ["tail", "-f", "/dev/null"]
