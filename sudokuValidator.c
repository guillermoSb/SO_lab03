#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>


// Global variables
int sudoku[9][9];


// Utility Functions
void
exitWithError(char* error, int errorCode) {
	printf("%s\n", error);
	exit(errorCode);
}


// Validations


// Main Program
int 
main(int argc, char* argv[]) {
	// Define variables
	int fd;
	char *fileName;
	char *addr;
	// Initial Validations
	if (argv[1] == NULL) exitWithError("El programa necesita un archivo a analizar.", 1);
	fileName = argv[1];
	// Read the file
	fd = open(fileName, O_RDONLY);
	if (fd == -1) exitWithError("El archivo no se pudo abrir.", 2);
	ftruncate(fd, sizeof(int) * 81);
	addr = mmap(NULL, sizeof(int) * 81, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (addr == MAP_FAILED)
	{
		exitWithError("Error al mapear la dirección de memoria." ,3);
	}

	// Fill the grid 
	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			int item = addr[(i * 9) + j] - '0';
			sudoku[i][j] = item;
		}
	}


	// Close fd and unmap
	close(fd);
	munmap(addr, 324);
}
