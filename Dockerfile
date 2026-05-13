FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc make curl \
    && curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Build React frontend (output -> backend/data/www)
WORKDIR /app/frontend
RUN npm install && npm run build

# Download mongoose and build C backend
WORKDIR /app/backend
RUN curl -sO https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h && \
    curl -sO https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c && \
    make

# Ensure data directories exist
RUN mkdir -p data/uploads data/contracts data/reports data/www

EXPOSE 8000

# Update STATIC_DIR to serve the built frontend
ENV STATIC_DIR=./data/www

CMD ["./swifttender"]
