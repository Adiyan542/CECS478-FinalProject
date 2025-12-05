bootstrap:
	@echo "Building all containers..."
	@docker-compose build

run:
	@docker-compose up

clean:
	@docker-compose down --remove-orphans

up: 
	@echo "Building and running the docker container..."
	docker compose -f src/docker-compose.yml up -d --build --force-recreate

demo:
	@echo "Running the network conversation demo"
	python3 demo_script.py
