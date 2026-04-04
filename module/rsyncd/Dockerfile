FROM alpine
EXPOSE 873
RUN apk add python3 rsync
COPY entrypoint.sh /entrypoint.sh
COPY genconf.py /genconf.py
COPY motd.txt /etc/rsyncd.motd
RUN chmod +x entrypoint.sh
CMD ["/entrypoint.sh"]
