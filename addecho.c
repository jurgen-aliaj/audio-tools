#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

/*
* Prints an informative message on how to use the program and exits.
*/
void print_usage(char **argv) {
	fprintf(stderr, "Usage: %s [-d delay] [-v volume_scale] sourcewav destwav\n", argv[0]);
	exit(1);
}

/*
* Prints information on what types are not allowed and exits.
*/
void print_types(char *which_arg) {
	fprintf(stderr, "Expected positive integer for arg %s\n", which_arg);
	exit(1);
}

int main(int argc, char **argv) {

	#define HEADER_SIZE 22
	#define DEFAULT_DELAY 8000
	#define DEFAULT_VOLUME 4

	FILE *f, *g, *f_copy; // Input, output, and copy of the input files, respectively
	int delay = DEFAULT_DELAY, volume_scale = DEFAULT_VOLUME;
	short *orig_buffer; // Declare two buffers, one that lags behind the first
	short *echo_buffer;
	short header[HEADER_SIZE]; // Store the header here
	char *endptr; // This is for storing result of strtol, it is unused
	int opt;
	int dflag = 0, vflag = 0;
	int start_delay = 0; // Keeps track if the echo has started or not, initially set to false
	int elem_copied; // Keeps track of number of elements copied to the output file
	int j; // Counter variable

	/*
	* Loops through all optional args, setting up the appropriate variables.
	*/
	while((opt = getopt(argc, argv, "d:v:")) != -1) {
		switch(opt) {
			case 'd': // Handle the delay optional argument
				if (dflag) {
					print_usage(argv); // Can't have two "-d's" 
				}
				delay = strtol(optarg, &endptr, 10); // Convert argument to long in base 10
				if (!delay)
					print_types("-d"); // We require a positive integer
				dflag++;
				break;
			case 'v': // Handled the same way as above
				if (vflag) {
					print_usage(argv);
				}
				volume_scale = strtol(optarg, &endptr, 10);
				if (!volume_scale) 
					print_types("-v");
				vflag++;
				break;
			default: // '?'
				print_usage(argv); // Only -d and -v are valid optional args
		}
	}

	if  (argc != 3 + (dflag + vflag)*2) {
		print_usage(argv); // Need either 3, 5, or 7 arguments
	}

	if ((orig_buffer = malloc(delay * sizeof(short))) == NULL || (echo_buffer = malloc(delay * sizeof(short))) == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		exit(1);
	}
	
	if ((f = fopen(argv[(dflag + vflag)*2 + 1], "r")) == NULL || (g = fopen(argv[(dflag + vflag)*2 + 2], "w")) == NULL) {
		fprintf(stderr, "Opening one or more of the given files resulted in an error\n");
		exit(1);
	}
	else {
		f_copy = fopen(argv[(dflag + vflag)*2 + 1], "r"); // Open the copy otherwise
	}

	if (fread(header, sizeof(short), HEADER_SIZE, f) != HEADER_SIZE) {
		fprintf(stderr, "The file %s does not contain an appropriate header\n", argv[(dflag + vflag)*2 + 1]);
		exit(1);
	}

	if (fseek(f_copy, HEADER_SIZE * sizeof(short), SEEK_SET) != 0) { // skip ahead in the copy past the header
		fprintf(stderr, "Unable to skip past header in the file: %s\n", argv[(dflag + vflag)*2 + 1]);
		exit(1);
	} 
	
	unsigned int *sizeptr1 = (unsigned int *)(header + 2);
	*sizeptr1 += delay * sizeof(short); // Modify the header to specify the correct size
	unsigned int *sizeptr2 = (unsigned int *)(header + 20);
	*sizeptr2 += delay * sizeof(short); // Modify the header to specify the correct size

	if (fwrite(header, sizeof(short), HEADER_SIZE, g) != HEADER_SIZE) {
		perror(argv[(dflag + vflag)*2 + 2]); // Error writing to the file, so print a message and exit
		exit(1);
	}

	/*
	* Loop over the input file so long as delay amount of samples are read,
	* and copy the original file and the echo to the output file appropriately.
	*/
	while ((elem_copied = fread(orig_buffer, sizeof(short), delay, f)) == delay) {
		
		if (start_delay) { // If we started echoing then start mixing the samples appropriately
			if (fread(echo_buffer, sizeof(short), delay, f_copy) != delay) {
				fprintf(stderr, "Error reading from file: %s\n", argv[(dflag + vflag)*2 + 1]);
				exit(1);
			}
			for(j = 0; j < delay; j++) {
				orig_buffer[j] += echo_buffer[j]/volume_scale;
			}
		}
		else { // If we haven't started echoing, then we will during the next loop iteration
			start_delay = 1;
		}

		// Write the buffer to the file
		if (fwrite(orig_buffer, sizeof(short), delay, g) != delay) {
			fprintf(stderr, "Error writing to file: %s\n", argv[(dflag + vflag)*2 + 2]);
			exit(1);
		}
	}

	/*
	* We want to ensure that our loop ended because of an end of file error,
	* rather than some arbitrary error. So we need to check ferror.
	*/
	if (ferror(f)) {
		fprintf(stderr, "Error reading from file: %s\n", argv[(dflag + vflag)*2 + 1]);
		exit(1);
	}

	/*
	* The idea here is that we want to check whether or not we actually started echoing or not.
	* This way, we will know if the file was shorter than delay in which case we need to add 0's to the file.
	*/
	if (start_delay) {

		if (fread(echo_buffer, sizeof(short), delay, f_copy) != delay) {
			fprintf(stderr, "Error reading from file: %s\n", argv[(dflag + vflag)*2 + 1]);
			exit(1);
		}

		// Add the remaining mixed samples
		for (j = 0; j < elem_copied; j++) {
			orig_buffer[j] += echo_buffer[j]/volume_scale;
		}

		// Add the remaining echoes
		for (j = elem_copied; j < delay; j++) {
			orig_buffer[j] = echo_buffer[j]/volume_scale;
		}

		// Write the remaining mixed samples
		if (fwrite(orig_buffer, sizeof(short), delay, g) != delay) {
			fprintf(stderr, "Error writing to file: %s\n", argv[(dflag + vflag)*2 + 2]);
			exit(1);
		}

		free(orig_buffer); // Free the allocated memory, don't need it anymore

		if ((echo_buffer = realloc(echo_buffer, elem_copied * sizeof(short))) == NULL) {
			fprintf(stderr, "Memory allocation error\n");
			exit(1);
		}

		if (fread(echo_buffer, sizeof(short), elem_copied, f_copy) != elem_copied) {
			fprintf(stderr, "Error reading from file: %s\n", argv[(dflag + vflag)*2 + 1]);
			exit(1);
		}

		for (j = 0; j < elem_copied; j++) {
			echo_buffer[j] /= volume_scale;
		}

		// Write the remaining echoes
		if (fwrite(echo_buffer, sizeof(short), elem_copied, g) != elem_copied) {
			fprintf(stderr, "Error writing to file: %s\n", argv[(dflag + vflag)*2 + 2]);
			exit(1);
		}

		free(echo_buffer); // Free the allocated memory, don't need it anymore
	}
	else {

		// Add a bunch of zeros
		for (j = elem_copied; j < delay; j++) {
			orig_buffer[j] = 0;
		}

		// Write the samples
		if (fwrite(orig_buffer, sizeof(short), delay, g) != delay) {
			fprintf(stderr, "Error writing to file: %s\n", argv[(dflag + vflag)*2 + 2]);
			exit(1);
		}

		free(orig_buffer); // Free the allocated memory, don't need it anymore

		if ((echo_buffer = realloc(echo_buffer, elem_copied * sizeof(short))) == NULL) {
			fprintf(stderr, "Memory allocation error\n");
			exit(1);
		}

		if (fread(echo_buffer, sizeof(short), elem_copied, f_copy) != elem_copied) {
			fprintf(stderr, "Error reading from file: %s\n", argv[(dflag + vflag)*2 + 1]);
			exit(1);
		}

		for (j = 0; j < elem_copied; j++) {
			echo_buffer[j] /= volume_scale;
		}

		// Write the remaining echoes
		if (fwrite(echo_buffer, sizeof(short), elem_copied, g) != elem_copied) {
			fprintf(stderr, "Error writing to file: %s\n", argv[(dflag + vflag)*2 + 2]);
			exit(1);
		}

		free(echo_buffer); // Free the allocated memory, don't need it anymore
	}

	fclose(f);
	fclose(g);

	return 0;
}
