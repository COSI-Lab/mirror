# Project Structure

## mirror-website
The `mirror-website` module serves the website. It uses a reverse proxy (NGINX)
to host static files. It is a pretty small Python program that uses Django to
generate and cache pages. In production, we use the 
[gunicorn](https://gunicorn.org/) WSGI server.

## mirror-logging
The `mirror-logging` directory contains minimal logging libraries for C++ and
Java.

## mirror-map
The `mirror-map` module serves the Map websocket. It gets information about
activity on Mirror by connecting to `mirror-metrics`'s 0MQ PUB socket. It is
written in Java.

## mirror-metrics
The `mirror-metrics` module tails the NGINX log and publishes metrics for
the Prometheus instance to scrape. It also publishes events on a 0MQ PUB socket
that other modules can subscribe to. It is written in C++.

## mirror-sync-scheduler
The `mirror-sync-scheduler` module keeps our mirrors up to date by running
rsyncs or scripts at scheduled times. It is written in C++.

## mirror-torrent-handler
The `mirror-torrent-handler` module finds torrents in our mirrors and links them
to be seeded. It is written in Java.

> The torrent handler is currently not in production. Mirror's bandwidth is very
limited, so we will likely do more good by seeding torrents with other servers
that have bandwidth to spare.
