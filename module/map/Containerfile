# Build
FROM eclipse-temurin:21 as builder
RUN apt update && apt install -y maven
WORKDIR /mirror
COPY ./pom.xml /mirror/pom.xml
RUN mvn verify --fail-never
COPY ./src /mirror/src
RUN mvn package -DskipTests

# Run
FROM eclipse-temurin:21
EXPOSE 8080
WORKDIR /mirror
COPY --from=builder /mirror/target/mirror-map-*-jar-with-dependencies.jar ./mirror-map.jar
ENTRYPOINT [ "java", "-jar", "./mirror-map.jar" ]
