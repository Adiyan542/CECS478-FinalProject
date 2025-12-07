bootstrap:
	@echo "Building all containers..."
	make up && make demo

run:
	@echo "Running the demo..."
	make demo

clean:
	@echo "Cleaning up all containers..."
	docker-compose -f src/docker-compose.yml down --remove-orphans

up: 
	@echo "Building and running the docker container..."
	docker compose -f src/docker-compose.yml up -d --build --force-recreate

demo:
	@echo "Running the network conversation demo (happy-test)"
	python3 scripts/demo_script.py
	@echo "Running the network conversation demo (negative-test)"
	python3 scripts/demo_script_negative.py
