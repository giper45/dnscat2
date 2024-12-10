# Docker Example
In this example, a server and a client are configured through Docker.
To understand how everything works, see the `Dockerfile`s in server and client.

## Usage
Run the stack with: 
```bash
make up
``` 

it starts the client and server. Actually, no dnscat services are running. 

Once the stack is executed, run the server with:
```bash
make run-server
```

Now, you have a `dnscat2` DNS server and a shell in the container.

The third step is to run the client: 
```bash
make run-client
```

With this command, the client connects itself to the server.

### Environment variables
For the server:
- `DNSCAT_SECRET`: the secret that should be used by the client.
- `DNS_OPTS`: the `dns` options 

For the client:
- `DNSCAT_SECRET`: the secret that should be used
- `DNSCAT_SERVER`: the dnscat2 server IP address.