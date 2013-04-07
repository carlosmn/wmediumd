#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include "config.h"
#include "wmediumd.h"

/* This is to make config.c happy */
double *prob_matrix = NULL;
int size = 0;
struct jammer_cfg jam_cfg;

char *xstrdup(const char *in)
{
	char *ret = strdup(in);

	if (!ret) {
		perror("strdup");
		exit(EXIT_FAILURE);
	}

	return ret;
}

void help()
{
	printf("usage: write-config [-l <loss>] [-n <number of ifaces>] -o <output file>");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int opt, num = 10;
	double loss = 0.0;
	char *filename = NULL;


	while((opt = getopt(argc, argv, "n:o:l:")) != -1) {
		switch(opt) {
		case 'n':
			num = atoi(optarg);
			break;
		case 'o':
			filename = xstrdup(optarg);
			break;
		case 'l':
			loss = atof(optarg);
			break;
		}
	}

	if (!filename)
		help();

	write_config(filename, num, loss);

	return EXIT_SUCCESS; 
}
