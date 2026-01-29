FROM python:3.12-bookworm
EXPOSE 8000
RUN pip install django gunicorn pyzmq
RUN useradd --user-group --create-home mirror
USER mirror
WORKDIR /home/mirror
COPY --chown=mirror:mirror ./mirror_website ./mirror_website
COPY --chown=mirror:mirror ./manage.py .
STOPSIGNAL SIGINT
CMD ["gunicorn", "-w 8", "--threads=4", "-t 30", "-b 0.0.0.0:8000", "mirror_website.wsgi"]
