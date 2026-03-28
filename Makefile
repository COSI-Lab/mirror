dev:
	docker compose -f compose.dev.yaml up --build -d --remove-orphans
prod:
	docker compose -f compose.prod.yaml up --build -d --remove-orphans
down:
	docker compose -p mirror down
test:
        make down && git pull && make prod && sleep 2 && docker ps
certbot:
	sudo apt-get remove certbot
	sudo snap install --classic certbot
	make down
	make dev
	certbot certonly --webroot
	make down
	@echo If everything worked you should be all set to run `make prod`