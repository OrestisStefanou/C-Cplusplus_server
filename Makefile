all: program1 program2

program1: master.c
	gcc -o master master.c

program2: worker.c
	gcc -o worker worker.c

clean:
	rm worker master
	rm -r logfiles
