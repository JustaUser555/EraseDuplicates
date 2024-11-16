#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#ifdef _WIN32
	#include <windows.h>
	#include <direct.h>

	#define error print_windows_error
	#define NULL_FD NUL
	#define FILENO _fileno
#else
//	#include <unistd.h>
	#include <dirent.h>
	#include <sys/types.h>
//	#include <sys/stat.h>
//	#include <fcntl.h>

	#define error perror
	#define NULL_FD "/dev/null"
	#define FILENO fileno
#endif

int8_t opCode = 0;

void print_help_msg(){
	printf("Usage: [arguments] [options]\n");
	printf("Options:\n");
	printf("-h\t\t--help\t\t\tShow this help message\n");	
	printf("-s\t\t--show\t\t\tList duplicates wihout deleting\n");	
	printf("-d\t\t--delete\t\tDelete duplicates* [default]\n");
	printf("\t\t--delete=all\t\tDelete duplicates that are not found in the main directory**\n");			
	printf("-r\t\t--recursive\t\tSearch subdirectories\n");	
	printf("-i\t\t--interactive\tPrompt user to delete files, if file deletion is enabled [default]\n");
	printf("\n");
	printf("*Duplicates are always deleted from the second directory onwards, the duplicates in the first directory will be kept.\n");
	printf("**When searching for duplicates in more than two directories, the --delete=all command will check for all duplicates,\n");
	printf("and not only those who are found in the first directory. The duplicates will remain in the first folder they are found,\n");
	printf("following an alphabetical order.\n");
}

//Error handling for windows, output is redirected to stderr
//Retrieves error information, as perror does not work for windows-specific tasks
#ifdef _WIN32
	void print_windows_error(const char *msg) {
	    DWORD errorCode = GetLastError();
	    char errorBuffer[256];
	    
	    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	                  NULL, errorCode, 0, errorBuffer, sizeof(errorBuffer), NULL);

	    fprintf(stderr, "%s: %s\n", msg, errorBuffer);
	}
#endif

//Moves all traffic from a source to NULL
void silence_output(FILE* source){
	freopen(NULL_FD, "w", source);
}

//Redirects output from a source to a destination
void redirect_output(FILE *source, FILE *destination) {
    fflush(source);
    dup2(FILENO(destination), FILENO(source));
    setvbuf(source, NULL, _IONBF, 0);
}

//Finds a directory, but returns only a bool
bool find_dir(char* path){
	#ifdef _WIN32
		DWORD attributes;
		if(attributes = GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES){
			error(path);
			return false;
		}
		if(attributes & FILE_ATTRIBUTE_DIRECTORY != 0){
			return true;
		}
		return false;
	#else
		DIR* dir = opendir(path);
		
		if(dir == NULL){
			error(path);
			return false;
		}
		
		closedir(dir);
		return true;
	#endif
}

//Checks whether an argument is a path
bool arg_is_path(const char* arg){
	if(arg[0] == '-') return false;
	return true;
}

//Returns opCode values in accordance with the given arguments
int arg_is_option(const char* arg){
	if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0){
		return 1;
	} else if(strcmp(arg, "-s") == 0 || strcmp(arg, "--show") == 0){
		return 2;
	} else if(strcmp(arg, "-d") == 0 || strcmp(arg, "--delete") == 0){
		return 4;
	} else if(strcmp(arg, "-r") == 0 || strcmp(arg, "--recursive") == 0){
		return 8;
	} else if(strcmp(arg, "-i") == 0 || strcmp(arg, "--interactive") == 0){
		return 16;
	} else if(strcmp(arg, "--delete=all") == 0){
		return 32;
	} else {
		printf("%s does not match any options\n", arg);
		return 255;
	}
}

//Checks whether the arguments are valid, and if directories exist
bool check_args(int argc, char** argv){
	if(argc < 3){
		printf("Not enough arguments specified\n");
		return false;
	}
	if(!find_dir(argv[1]) || !find_dir(argv[2])){
		printf("Error finding directories: ABORT\n");
		return false;
	}

	//int8_t opCode = 0;
	for(int i = 3; i < argc; i++){
		if(arg_is_path(argv[i])){
			if(!find_dir(argv[i])) return false;
		}
		else opCode = opCode | arg_is_option(argv[i]);
		printf("OPCODE: %d", opCode);
	}
	if(opCode == 255){
		return false;
	}
	return true;
}

void convertToBinary(unsigned a) {
  if (a > 1) convertToBinary(a / 2);
  printf("%d", a % 2);
}

void print_bin(int8_t localOpCode){
	convertToBinary(localOpCode);
	printf("\n");
}

int main(int argc, char** argv){
	//const FILE* originalStdout = fdopen(dup(FILENO(stdout)), "w");
	//const FILE* originalStderr = fdopen(dup(FILENO(stderr)), "w");

	if(!check_args(argc, argv)){
		//print_bin(opCode);
		//return EXIT_FAILURE;
	}
	print_bin(opCode);
	return EXIT_SUCCESS;
}
