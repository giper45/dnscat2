up:
	@docker compose up -d --build 

down:
	@docker compose down

run-server: up
	@docker compose exec dnscat-server /app/run.sh
	
run-client: up
	@docker compose exec dnscat-client /app/run.sh