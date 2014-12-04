
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


extern int describe_HOG(const char*);
extern int describe_pHash(const char*);

int _verbose = 0;

void help(char** argv)
{
	printf("USAGE:\n\t%s -h | -p [-d path | --dataset-path path] "
		"[-v] [--help]\n", argv[0]);
}

int main(int argc, char** argv)
{
	int helpflag = 0;
	int errflag = 0;

	char* dataset_path = ".";
	int algorithm = 0;

	struct option loptions[] = {
		{"dataset-path", required_argument, NULL, 'd'},
		{"help", no_argument, &helpflag, 1},
		{0,0,0,0}
	};

	int opt;
	while((opt = getopt_long(argc, argv, "d:hpv", loptions, NULL)) != -1)
	{
		switch (opt)
		{
			case 'd':
				dataset_path = optarg;
				break;

			case 'h':
				algorithm = 1;
				break;

			case 'p':
				algorithm = 2;
				break;

			case 'v':
				_verbose = 1;
				break;

			case '?':
				errflag = EXIT_FAILURE;
				break;
		}
	}

	if(errflag != 0 || helpflag == 1)
	{
		help(argv);
		return errflag;
	}

	if(_verbose)
		printf("Dataset path set to \"%s\"\n", dataset_path);

	switch(algorithm)
	{
		case 1:
			if(_verbose)
				printf("Using HOG descriptor.\n");
			return describe_HOG(dataset_path);
			break;

		case 2:
			if(_verbose)
				printf("Using perceptive hash\n");
			return describe_pHash(dataset_path);
			break;

		default:
		case 0:
			printf("COMMAND-LINE ERROR: no algorithm specified.\n");
			help(argv);
			return EXIT_FAILURE;
			break;
	}

	return EXIT_SUCCESS;
}

