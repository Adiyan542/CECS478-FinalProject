bootstrap:
	@echo "Building all containers..."
	@docker-compose build

run:
	@docker-compose up

clean:
	@docker-compose down --remove-orphans
