#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>

#define error perror
#define NULL_FD "/dev/null"
#define FILENO fileno
#define MAX_PATH_LEN 256
#define ARR_SIZE 5

bool flag_Help = false;
bool flag_Show = false;
bool flag_Delete = false;
bool flag_Cross_Dir = false;
bool flag_Main_Dir = false;
bool flag_Recursive = false;
bool flag_Interactive = false;

void print_help_msg(){
	printf("Usage: [arguments] [options]\n");
	printf("Options:\n");
	printf("Short\tLong\t\tDefault\tExplanation\n");
	printf("-h\t--help\t\tNo\tShow this help message\n");	
	printf("-s\t--show\t\tYes\tList duplicates wihout deleting\n");	
	printf("-d\t--delete\tYes\tDelete duplicates*\n");
	printf("-m\t--maindir\tYes\tCheck against the main directory only\n");
	printf("-c\t--crossdir\tNo\tCross-search duplicates between every directory.**\n");			
	printf("-r\t--recursive\tNo\tSearch subdirectories\n");	
	printf("-i\t--interactive\tYes\tPrompt user to delete files, if file deletion is enabled\n");
	printf("\n");
	printf("*Duplicates are always deleted from the second directory onwards, the duplicates in the first directory will be kept. ");
	printf("**When searching for duplicates in more than two directories, the --crossdir command will check for all duplicates, ");
	printf("and not only those who are found in the first directory. The duplicates will remain in the first folder they are found, ");
	printf("following the order of input.");
	printf("Attention: Default values are applied only if no option is specified.\n");
}

//if the path contains mutliple directories
//than only the last file is being returned
//exp: path 	= /this/is/a/dir/test.c 
//	   return 	= test.c
char* file_of_path(char* path){
	char* delimiter = "/";

	for(int i = strlen(path) - 1; i >= 0; i--){
		if(path[i] == delimiter[0]){
			return &path[i+1];
		}
	}

	return path;
}

//concatenates two paths with simple logic
char* concatenate_paths(char* path1, char* path2){
	char* delimiter = "/";

	int len = strlen(path1);
	if(path1[len - 1] == delimiter[0]){
		strcat(path1, path2);
	} else {
		for(size_t i = 0; i < strlen(delimiter); i++){
			path1[len] = delimiter[i];
			len++;
		}
		path1[len] = '\0';
		strcat(path1, path2);
	}

	return path1;
}

//concatenates two paths with simple logic
//the returned pointer is a static string
//being overwritten everytime the function is called
char* concatenate_paths_temp(char* path1, char* path2){
	char* delimiter = "/";

	static char concat_path[MAX_PATH_LEN] = {0};
	memset(concat_path, 0, MAX_PATH_LEN);

	int i = 0;
	while(path1[i] != '\0'){
		concat_path[i] = path1[i];
		i++;
	}
	if(path1[strlen(path1) - 1] != delimiter[0]){
		int k = 0;
		while(delimiter[k] != '\0'){
			concat_path[i] = delimiter[k];
			k++;
			i++;
		}	
	}		
	int j = 0;
	while(path2[j] != '\0'){
		concat_path[i] = path2[j];
		j++;
		i++;
	}

	return concat_path;
}

//prints the received message and parses the input (y/N)
//to return true or false
bool handle_interactive(const char* msg){
	char buffer[MAX_PATH_LEN] = {0};
	for(int i = 0; i < 3; i++){
		printf("%s [y/n]: ", msg);
		if (!fgets(buffer, sizeof(buffer), stdin)) break;
		buffer[strcspn(buffer, "\n")] = '\0';
		if(strcmp(buffer, "y") == 0|| strcmp(buffer, "yes") == 0) return true;
		else if(strcmp(buffer, "n") == 0 || strcmp(buffer, "no") == 0) return false;
		else printf("Invalid input: %s\n", buffer);
	}
	printf("Terminating program\n");
	
	return false;
}

//prints a c string formatted array, i.e. ending with \0
void print_string_array(char** array, int rows){
	for(int i = 0; i < rows; i++){
		printf("%d %s\n", i, array[i]);
	}		
}

//same function as the above
//but doesn't print if the string is empty
//and doesn't print the iterator value
void print_string_array_if_full(char** array, int rows){		
	for(int i = 0; i < rows; i++){
		if(array[i][0] != '\0'){
			printf("%s\n", array[i]);
		}
	}
}

//Finds a directory, but returns only a bool
bool find_dir(char* path){	
	DIR* dir = opendir(path);
	
	if(dir == NULL){
		error(path);
		return false;
	}
	
	closedir(dir);
	return true;
}

//Checks whether an argument is a path
bool arg_is_path(const char* arg){
	if(arg[0] == '-') return false;
	return true;
}

//Sets the corresponding operation flags
bool arg_is_option(const char* arg){
	if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0){
		flag_Help = true;
	} else if(strcmp(arg, "-s") == 0 || strcmp(arg, "--show") == 0){
		flag_Show = true;
	} else if(strcmp(arg, "-d") == 0 || strcmp(arg, "--delete") == 0){
		flag_Delete = true;
	} else if(strcmp(arg, "-r") == 0 || strcmp(arg, "--recursive") == 0){
		flag_Recursive = true;
	} else if(strcmp(arg, "-i") == 0 || strcmp(arg, "--interactive") == 0){
		flag_Interactive = true;
	} else if(strcmp(arg, "-m") == 0 || strcmp(arg, "--maindir") == 0){
		flag_Main_Dir = true;
	} else if(strcmp(arg, "-c") == 0 || strcmp(arg, "--crossdir") == 0){
		flag_Cross_Dir = true;
	} else {
		printf("%s does not match any options\n", arg);
		return false;
	}
	return true;
}

//Checks whether the arguments are valid, and if directories exist
//Receives argc and argv, and copies path arguments into paths, and their count into count
//options are not copied, as they are globally set
//returns a bool, true if everything went alright, otherwise false
bool get_args(int argc, char** argv, char** paths, int* count){
	if(argc < 2){
		printf("No arguments specified\n");
		return false;
	}
	
	int j = 0;
	for(int i = 1; i < argc; i++){
		if(arg_is_path(argv[i])){
			if(find_dir(argv[i])){
				paths[j] = argv[i];
				j++;
			}
			else return false;
		}
		else if(!arg_is_option(argv[i])) return false;
	}
	*count = j;	

	return true;
}

//is NOT responisble for the main argument parsing/checking
//if no argument is given, it sets the default arguments
//in addition, if no operation mode has been selected,
//the program stops.
bool check_args(){
	if(!(flag_Show || flag_Delete || flag_Cross_Dir || flag_Main_Dir || flag_Recursive || flag_Interactive)){
		flag_Show = true;
		flag_Delete = true;
		flag_Interactive = true;
		flag_Main_Dir = true;
		printf("Using default values\n");
		return true;
	}

	if(flag_Main_Dir && flag_Cross_Dir){
		printf("Error: '--maindir' and '--crossdir' can't be combined.\n");
		return false;
	}

	if(!(flag_Main_Dir || flag_Cross_Dir)){
		printf("Using --maindir mode\n");
		flag_Main_Dir = true;
	}

	return true;
}

//calculates the array size (of lists) when --crossdir option is chosen
int calc_multiplier(int count){
	count--;
	int dupCount = count;
	for(int i = 1; i < count; i++){
		dupCount += count - i;
	}
	return dupCount;
}

//allocates a 2d char array
char** alloc_string_array(int rows, int cols){
	char** array = (char**)malloc(sizeof(char*) * rows);
	if(array == NULL){
		error("malloc");
		return NULL;
	}
	for(int i = 0; i < rows; i++){
		array[i] = (char*)malloc(sizeof(char) * cols);
		if(array[i] == NULL){
			error("malloc");
			for(int j = 0; j < i; j++) free(array[i]);
			free(array);
			return NULL;
		}
		if(memset(array[i], 0, cols) == NULL){
			error("memset");
			for(int j = 0; j < i; j++) free(array[i]);
			free(array);
			return NULL;
		}
	}
	return array;
}

//copies a 2d char array into another 2d char array
//uses the defined MAX_PATH_LEN for string lenght
//the size of dest must be big enough;
//if you want to copy from a point in the array to another one,
//specify offset as the start element, otherwise set it to zero
char** copy_string_arr(char** dest, char** src, int size, int destOffset, int srcOffset){
	for(int i = 0; i < size; i++){
		strcpy(dest[destOffset + i], src[srcOffset + i]);
	}
	return dest;
}

//reallocates a 2d char array
//does not reallocate old char arrays (strings) to a new lenght
//new strings will have the new length specified, though
//to enable that feature uncomment the lines below
char** realloc_string_array(char** ptr, int newRows, int newCols, int oldRows/*, int oldCols*/){
	char** temp = (char**)realloc(ptr, newRows * sizeof(char*));
	if(temp == NULL){
		error("realloc");
		return NULL;
	}
	//code not checked yet
	//should be placed before reallocating the char** pointers,
	//as the ptr* may have been freed by the realloc above
	/*if(newCols > oldCols){
		for(int i = 0; i < oldRows; i++){
			temp[i] = (char*)realloc(ptr[i], sizeof(char) * newCols);
			if(temp[i] == NULL){
				perror("realloc");
				return NULL;
			}
		}
	}*/
	for(int i = oldRows; i < newRows; i++){
		temp[i] = (char*)malloc(sizeof(char) * newCols);
		if(temp[i] == NULL){
			error("malloc");
			for(int j = 0; j < i; j++) free(temp[i]);
			free(temp);
			return NULL;
		}
		if(memset(temp[i], 0, newCols) == NULL){
			error("memset");
			for(int j = 0; j < i; j++) free(temp[i]);
			free(temp);
			return NULL;
		}
	}
	ptr = temp;
	return ptr;
}

void free_string_array(char** ptr, int size){
	for(int i = 0; i < size; i++){
		free(ptr[i]);
	}
	free(ptr);
}

//receives 2 string arrays and deletes the duplicates from the second array
//returns the second array
char** delete_duplicate_strings_different_arrays(char** stringArr1, int size1, char** stringArr2, int size2){
	for(int i = 0; i < size1; i++){
		for(int j = 0; j < size2; j++){
			if(strcmp(stringArr1[i], stringArr2[j]) == 0){
				memset(stringArr2[j], 0, MAX_PATH_LEN);
				for(int k = j; k < size2 - 1; k++){
					strcpy(stringArr2[k], stringArr2[k+1]);
				}
				memset(stringArr2[size2-1], 0, MAX_PATH_LEN);
			}		
		}
	}

	return stringArr2;
}

//receives a string array and deletes duplicates in that array
//returns the array
char** delete_duplicate_strings_same_array(char** stringArr, int size){
	for(int i = 0; i < size; i++){
		for(int j = i + 1; j < size; j++){
			if(strcmp(stringArr[i], stringArr[j]) == 0){
				memset(stringArr[j], 0, MAX_PATH_LEN);
				for(int k = j; k < size - 1; k++){
					strcpy(stringArr[k], stringArr[k+1]);
				}
				memset(stringArr[size-1], 0, MAX_PATH_LEN);
			}		
		}
	}
	
	return stringArr;
}

//receives an array of string arrays and deletes duplicates in them
//it does not delete duplicates in an array of strings, but only
//between the arrays of strings, as the duplicates in the array of
//strings have been deleted beforehand.
char*** delete_duplicate_strings_3d(char*** stringArr, int size, int* sizeList){
	for(int i = 0; i < size; i++){
		for(int j = 0; j < sizeList[i]; j++){
			for(int k = i + 1; k < size; k++){
				for(int l = 0; l < sizeList[k]; l++){
					if(strcmp(stringArr[i][j], stringArr[k][l]) == 0){
						memset(stringArr[k][l], 0, MAX_PATH_LEN);
						for(int m = l; m < sizeList[k] - 1; m++){
							strcpy(stringArr[k][m], stringArr[k][m+1]);
						}
						memset(stringArr[k][sizeList[k] - 1], 0, MAX_PATH_LEN);
					}
				}
			}
		}
	}
	return stringArr;
}

//receives the array of duplicates (duplicate arrays) and compares the duplicates arrays
//which are being removed from the same directory, and calls the function to delete the
//duplicate duplicates. This is needed, so that in the deletion process, the program doesn't
//try to delete the same files again.
char*** delete_duplicate_duplicates(char*** stringArr, int* sizeList, int pathCount){
	pathCount--;

	int k, m;
	for(int i = 1; i < pathCount; i++){
		m = i;
		for(int l = 1; l <= i; l++){
			k = m;
			for(int j = 1; j <= i - (l - 1); j++){
				k = k + pathCount - j - (l-1);
				delete_duplicate_strings_different_arrays(stringArr[m], sizeList[m], stringArr[k], sizeList[k]);
			}
			m = m + pathCount - l;
		}
	}

	return stringArr;
}

void print_dup_maindir_test(char*** duplicates, int dirCount, int *arrSizes, char** paths){
	for(int i = 0; i < dirCount - 1; i++){
		printf("%s\n", paths[i+1]);
		print_string_array_if_full(duplicates[i], arrSizes[i]);
		printf("\n");
	}
}

void print_dup_crossdir_test(char*** duplicates, int dirCount, int *arrSizes, char** paths){
	dirCount--;
	
	for(int i = 0; i < dirCount; i++){
		int k = i;
		printf("%s\n", paths[i+1]);
		for(int j = 0; j <= i; j++){
			print_string_array_if_full(duplicates[k], arrSizes[k]);
			k += dirCount - j - 1;		
		}
		printf("\n");
	}
}

void print_duplicates(char*** duplicates, char** paths, int dirCount, int *arrSizes){
	if(flag_Main_Dir){
		printf("Checked against %s\n\n", paths[0]);
	}

	if(flag_Main_Dir){
		print_dup_maindir_test(duplicates, dirCount, arrSizes, paths);	
	} else {
		print_dup_crossdir_test(duplicates, dirCount, arrSizes, paths);
	}
}

//returns an array of the file names in the directory specified in the path variable
char** get_files(const char* path, int* size){

	char** newPtr = NULL;
	struct dirent* entry;
	char** files = alloc_string_array(*size, MAX_PATH_LEN);
	if(files == NULL) return NULL;

	DIR* dir = opendir(path);
	if(dir == NULL){
		error(path);
		free_string_array(files, *size);
		return NULL;
	}

	int i = 0;
	while((entry = readdir(dir))){
		//printf("LOOP\n");
		if(i >= *size){
			//printf("REALLOC\n");
			*size *= 2;
			if((newPtr = realloc_string_array(files, *size, MAX_PATH_LEN, *size/2)) == NULL){
				free_string_array(files, *size/2);
				closedir(dir);
				return NULL;
			}
			files = newPtr;
		}
		if(entry->d_type == DT_REG){
			//printf("ADD FILE: %d\n", i);
			strcpy(files[i], entry->d_name);
			i++;
		} else if(flag_Recursive && entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
			//printf("ADD DIR\n");
			char subdirPath[MAX_PATH_LEN] = {0};
			strcpy(subdirPath, path);
			concatenate_paths(subdirPath, entry->d_name);
			int subdirSize = ARR_SIZE;
			char** tempFileList = get_files(subdirPath, &subdirSize);
			if(tempFileList == NULL){
				free_string_array(files, *size);
				closedir(dir);
				return NULL;
			}
			if((newPtr = realloc_string_array(files, *size + subdirSize, MAX_PATH_LEN, *size)) == NULL){
				free_string_array(tempFileList, subdirSize);
				free_string_array(files, *size);
				closedir(dir);
				return NULL;
			}
			files = newPtr;
			copy_string_arr(files, tempFileList, subdirSize, *size, 0);
			free_string_array(tempFileList, subdirSize);
			for(int j = *size; j < *size + subdirSize; j++){
				if(files[j][0] != '\0'){
					char* temp = concatenate_paths_temp(entry->d_name, files[j]);
					//printf("CONCAT: %s\n", temp);
					strcpy(files[j], temp);	
				}			
			}
			*size += subdirSize;
			i = *size;
		}
	}
	//printf("FINISH\n\n");

	closedir(dir);

	//print_string_array(files, *size);

	return files;
}

//returns a list of duplicated files
char** search_duplicates(const char* path1, const char* path2, int* dupSize){
	int size1 = ARR_SIZE, size2 = ARR_SIZE;
	char** files1 = get_files(path1, &size1);
	if(files1 == NULL) return NULL;
	char** files2 = get_files(path2, &size2);
	if(files2 == NULL){
		free_string_array(files1, size1);
		return NULL;
	}

	if(size1 < size2) *dupSize = size1;
	else *dupSize = size2;
	
	char** duplicates = alloc_string_array(*dupSize, MAX_PATH_LEN);
	if(duplicates == NULL) return NULL;
	
	int k = 0;
	for(int i = 0; i < size1; i++){
		for(int j = 0; j < size2; j++){
			if(files1[i][0] != '\0' && strcmp(file_of_path(files1[i]), file_of_path(files2[j])) == 0){
				strcpy(duplicates[k], files2[j]);
				k++;
				if(!flag_Recursive){
					break;
				}
			}
		}
	}

	free_string_array(files1, size1);
	free_string_array(files2, size2);

	if(flag_Recursive){
		delete_duplicate_strings_same_array(duplicates, *dupSize);		
	}

	return duplicates;
}

char*** search_cross_dir(char** paths, int count, int** sizesList, char** duplicates, char*** lists){
	int l = 0;
	for(int i = 0; i < count - 1; i++){
		for(int j = i + 1; j < count; j++, l++){
			duplicates = search_duplicates(paths[i], paths[j], &(*sizesList)[l]);
			if(duplicates == NULL){
				for(int k = 0; k < l; k++){
					free_string_array(lists[k], (*sizesList)[k]);
				}
				free(lists);
				return NULL;
			}
			lists[l] = duplicates;
		}
	}

	return lists;
}

char*** search_main_dir(char** paths, int count, int** sizesList, char** duplicates, char*** lists){
	for(int i = 0; i < count - 1; i++){
		duplicates = search_duplicates(paths[0], paths[i+1], &(*sizesList)[i]);
		if(duplicates == NULL){
			for(int j = 0; j < i; j++){
				free_string_array(lists[j], (*sizesList)[j]);
			}
			free(lists);
			return NULL;
		}
		lists[i] = duplicates;
	}

	return lists;
}

char*** handle_search(char** paths, int count, int** sizesList){
	int multiplier = count - 1;
	if(flag_Cross_Dir){
		multiplier = calc_multiplier(count);
	}

	char** duplicates = NULL;
	*sizesList = (int*)malloc(sizeof(int) * multiplier);
	if(sizesList == NULL){
		error("malloc");
		return NULL;
	}
	char*** lists = (char***)malloc(sizeof(char**) * multiplier);
	if(lists == NULL){
		error("malloc");
		free(*sizesList);
		return NULL;
	}

	if(flag_Main_Dir){
		lists = search_main_dir(paths, count, sizesList, duplicates, lists);
	} else {
		lists = search_cross_dir(paths, count, sizesList, duplicates, lists);
		lists = delete_duplicate_duplicates(lists, *sizesList, count);
	}
	
	return lists;
}

bool remove_file(const char* filePath){
	if(remove(filePath) != 0){
		perror("Error deleting file");
		return false;
	}
	
	return true;
}

bool remove_cross_dir(char** paths, int pCount, char*** duplicates, int* dCount){
	int crossCount = calc_multiplier(pCount);

	int pCounter = 1;
	for(int i = 0; i < crossCount; i++){
		if(pCounter == pCount) pCounter = 1;
		for(int j = 0; j < dCount[i]; j++){
			if(duplicates[i][j][0] == '\0') break;
			char* filePath = concatenate_paths_temp(paths[pCounter], duplicates[i][j]);
			printf("I=%d\tJ=%d\t%s\n", i, j, filePath);
			if(!remove_file(filePath)) return false;
		}
		pCounter++;
	}

	return true;
}

bool remove_main_dir(char** paths, int pCount, char*** duplicates, int* dCount){
	for(int i = 0; i < pCount - 1; i++){
		for(int j = 0; j < dCount[i]; j++){
			if(duplicates[i][j][0] == '\0') break;
			char* filePath = concatenate_paths_temp(paths[i+1], duplicates[i][j]);
			if(!remove_file(filePath)) return false;
		}
	}

	return true;
}

int handle_removal(char** paths, int pCount, char*** duplicates, int* dCount){
	if(flag_Interactive && !handle_interactive("Would you like to delete all the found duplicates?")){
		return 2;
	}

	if(flag_Main_Dir){
		return remove_main_dir(paths, pCount, duplicates, dCount);
	} else {
		return remove_cross_dir(paths, pCount, duplicates, dCount);
	}
}

void free_all(char*** lists, int* sizesList, char** paths, int count){
	if(flag_Cross_Dir) count = calc_multiplier(count) + 1;

	for(int i = 0; i < count - 1; i++){
		free_string_array(lists[i], sizesList[i]);
	}
	free(lists);
	free(sizesList);
	free(paths);
}

int main(int argc, char** argv){
	int exitCode = EXIT_SUCCESS;

	char** paths = (char**)malloc(sizeof(char*) * argc);
	if(paths == NULL){
		error("malloc failure");
		return EXIT_FAILURE;
	}
	int count = 0;
	
	if(!get_args(argc, argv, paths, &count)) return EXIT_FAILURE;

	if(flag_Help){
		print_help_msg();
		return EXIT_SUCCESS;
	}

	if(count < 2){
		printf("Specifiy at least two paths\n");
		return EXIT_FAILURE;
	}

	if(!check_args()) return EXIT_FAILURE;

	
	
	char*** lists = NULL;
	int* sizesList = NULL;
	if((lists = handle_search(paths, count, &sizesList)) == NULL){
		printf("Exiting program: FAILURE SEARCHING FILES\n");
		return EXIT_FAILURE;
	}

	if(flag_Show){
		print_duplicates(lists, paths, count, sizesList);
	}

	if(flag_Delete){
		int check = handle_removal(paths, count, lists, sizesList);

		if(check == 0){
			printf("Deletion successful\n");
		} else if(check == 1){
			printf("Exiting program: FAILURE REMOVING FILES\n");
			exitCode = EXIT_FAILURE;
		} else if(check == 2){
			printf("Deletion aborted\n");
		}
	}

	free_all(lists, sizesList, paths, count);
	
	return exitCode;
}
