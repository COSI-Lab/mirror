FROM python:3.14-alpine
EXPOSE 8080
RUN addgroup -S mirror-map && adduser -S mirror-map -G mirror-map
USER mirror-map
RUN mkdir /home/mirror-map/map
WORKDIR /home/mirror-map/map
COPY ./requirements.txt /home/mirror-map/map
RUN python -m pip install -r requirements.txt
COPY ./GeoLite2-City_20260324 ./GeoLite2-City_20260324 
COPY ./src ./src
CMD ["python", "src/main.py"]