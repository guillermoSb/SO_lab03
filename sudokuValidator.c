#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
// #include <omp.h>

// Global variables
int sudoku[9][9];


// Utility Functions
void
exitWithError(char* error, int errorCode) {
	printf("%s\n", error);
	exit(errorCode);
}

// Validations 0 -> ok; 1 -> not ok
void* 
validateRows()
{
	for (int i = 0; i < 9; i++)
	{
		int sum = 0;
		#pragma omp parallel for private(sudoku)
		for (int j = 0; j < 9; j++)
		{
			if (sudoku[i][j] < 1 || sudoku[i][j] > 9) return 1;
			sum += sudoku[i][j];
		}
		if(sum != 45)	pthread_exit((void*) 1);
	}
	pthread_exit((void*) 0);
}


void* 
validateColumns()
{
	for (int j = 0; j < 9; j++)
	{
		int sum = 0;
		#pragma omp parallel for private(sudoku)
		for (int i = 0; i < 9; i++)
		{
			if (sudoku[i][j] < 1 || sudoku[i][j] > 9) pthread_exit((void*) 1);
			sum += sudoku[i][j];
		}
		if (sum != 45) pthread_exit((void*) 1);
	}
	printf("Numero del thread: %d\n", syscall(SYS_gettid));
	pthread_exit((void*) 0);
}

int 
validate3x3(int matrix[3][3])
{
	int sum = 0;
	#pragma omp parallel for private(matrix)
	for (int i = 0; i < 3; i++)
	{
		#pragma omp parallel for
		for (int j = 0; j < 3; j++)
		{
			sum += matrix[i][j];
		}
	}
	if (sum != 45)
		return 1;
	return 0;
}

// Main Program
int 
main(int argc, char* argv[]) {
	// omp_set_num_threads(1);
	// Define variables
	int fd;
	char *fileName;
	char *addr;
	pid_t parentProcessId;
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
	#pragma omp parallel for private(sudoku)
	for (int i = 0; i < 9; i++)
	{
		#pragma omp parallel for
		for (int j = 0; j < 9; j++)
		{
			int item = addr[(i * 9) + j] - '0';
			sudoku[i][j] = item;
		}
	}

	// Checking sub arrays
	#pragma omp parallel for private(sudoku)
	for (int i = 0; i < 9; i += 3)
	{
		#pragma omp parallel for
		for (int j = 0; j < 9; j+= 3)
		{
			int matrix[3][3] = {
					{sudoku[i][j], sudoku[i][j + 1], sudoku[i][j + 2]},
					{sudoku[i + 1][j], sudoku[i + 1][j + 1], sudoku[i + 1][j + 2]},
					{sudoku[i + 2][j], sudoku[i + 2][j + 1], sudoku[i + 2][j + 2]},
			};
			if (validate3x3(matrix) != 0) exitWithError("Solución inválida", 4);
		}
	}
	// Obtain the current process
	parentProcessId = getpid();

	int currentProcess = fork();
	if (currentProcess == 0)
	{
		char processStr[25033];
		sprintf(processStr, "-p %d", parentProcessId);
		execlp("ps", "ps", processStr, "-lf", NULL);
	} else {
		// Create pthread to review columns
		pthread_t columnThread;
		pthread_t rowsThread;
		int *resValidRows;
		int *resValidColumns;
		pthread_create(&columnThread, NULL, validateColumns, NULL);
		pthread_create(&rowsThread, NULL, validateRows, NULL);
		pthread_join(columnThread, (void**) &resValidColumns);
		printf("Numero del thread: %d\n", syscall(SYS_gettid));
		if (resValidColumns != 0)
			exitWithError("Las columnas no son válidas", 5);
		// Wait for child process
		wait(NULL);
		// Review rows
		pthread_join(rowsThread, (void**) &resValidRows);
		if (resValidRows != 0)
			exitWithError("Las filas no son válidas", 6);
		printf("Solución válida!!!\n");
		// Create another fork
		int secondProcess = fork();
		if (secondProcess == 0)
		{
			printf("Antes de terminar el estado de este proceso y sus threads es:\n");
			char processStr[25033];
			sprintf(processStr, "-p %d", parentProcessId);
			execlp("ps", "ps", processStr, "-lf", NULL);
		} else {
			// Wait for child
			wait(NULL);
		}
		
	}



	// Close fd and unmap
	close(fd);
	munmap(addr, 324);
}
