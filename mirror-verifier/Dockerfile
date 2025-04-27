# Build
FROM eclipse-temurin:21 as builder
RUN apt update && apt install -y maven
WORKDIR /mirror
COPY ./pom.xml /mirror/pom.xml
COPY ./src /mirror/src
RUN mvn package

# Run
FROM eclipse-temurin:21
EXPOSE 27887 27886
WORKDIR /mirror
COPY --from=builder /mirror/target/*-jar-with-dependencies.jar ./mirror-verifier.jar
ENTRYPOINT java -jar ./mirror-verifier.jar