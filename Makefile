dev:
	docker compose -f compose.dev.yaml up --build -d --remove-orphans
prod:
	docker compose -f compose.prod.yaml up --build -d --remove-orphans
down:
	docker compose -p mirror down
