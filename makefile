all: 
	gcc remvocals.c -o remvocals
	gcc addecho.c -o addecho

clean:
	rm -rf remvocals addecho
