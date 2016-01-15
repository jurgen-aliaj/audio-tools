#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

	#define HEADER_SIZE 22

	short header[HEADER_SIZE]; // We will store the header here
	short left_right[2]; // Store the left and right shorts to combine
	FILE *f, *g; // Input and output files, respectively
	
	/*
	* Prints an error and exits if precisely 3 arguments are not provided.
	*/
	if(argc != 3) {
		fprintf(stderr, "Usage: %s sourcewav destwav\n", argv[0]);
		exit(1);
	}	
	
	/*
	* Prints an error and exitw if we cannot open the files.
	*/
	if((f = fopen(argv[1], "r")) == NULL || (g = fopen(argv[2], "w")) == NULL) {
		fprintf(stderr, "Opening one or more of the given files resulted in an error\n");
		exit(1);
	}

	/*
	* We require a header size of at least 44 bytes.
	*/
	if (fread(header, sizeof(short), HEADER_SIZE, f) != HEADER_SIZE) {
		fprintf(stderr, "The file %s does not contain an appropriate header\n", argv[1]);
		exit(1);
	}

	/*
	* Prints an error and exits if we cannot write to the given output file.
	*/
	if (fwrite(header, 2, HEADER_SIZE, g) != HEADER_SIZE) {
		fprintf(stderr, "There was an error writing to the file %s\n", argv[2]);
		exit(1);
	}

	/*
	* Loops through the file and combines the left and right shorts,
	* writes two copies of the combined shorts to the output file.
	*/
	while(fread(left_right, sizeof(short), 2, f) == 2) {

		short combined = (left_right[0]-left_right[1])/2;

		left_right[0] = combined;
		left_right[1] = combined;

		// Prints an error and exits
		if (fwrite(left_right, sizeof(short), 2, g) != 2) {
			fprintf(stderr, "There was an error writing to the file %s\n", argv[2]);
			exit(1);
		}
	}

	/*
	* We check ferror to make sure our loop stopped because of an end of file error,
	* rather than some arbitrary error.
	*/
	if (ferror(f)) {
		fprintf(stderr, "Error reading file: %s\n", argv[1]);
		exit(1);
	}

	fclose(f);
	fclose(g);

	return 0;
}