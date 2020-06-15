all: program1 program2 program3 program4

program1: master.c
	gcc -o master master.c

program2: worker.c
	gcc -o worker worker.c

program3: WhoServer.c
	gcc -o WhoServer WhoServer.c -pthread

program4: Client.c 
	gcc -o whoClient Client.c -pthread
clean:
	rm worker master WhoServer whoClient
	rm -r logfiles
