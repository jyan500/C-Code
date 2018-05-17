#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
// I define max matches to be 4 in a single string
#define MAX_MATCHES 4 

int isRegularFile(char * path){
	struct stat st;
	if (stat(path, &st) < 0){
		return -1;
	}
	return S_ISREG(st.st_mode);
}
int isAllDigits(char * a){
	if (*a == '\0'){
		return 0;
	}
	while (*a){
		if (isdigit(*a) == 0){
			return 0;
		}
		a++;
	}
	return 1;
}
int main(int argc, char * argv[]){

	FILE* fp;
	char buf[1024];
	//const char * pattern = "^-([0-9]*),([0-9]*)$";
	const char * pattern2 = "^-([1-9][0-9]*)($|,([1-9][0-9]*$))";
	regex_t regex;
	int returnVal;
	char errMsg[128];
	char matchGroups[MAX_MATCHES][128];
	char patternStr[128];
	char ** fileNames;
	int fileNamesLen;
	int isFileFirst = 0;
	int fileNamesCounter = 0;
	regmatch_t matches[MAX_MATCHES];
	
	// If no arguments are given
	if (argc == 1){
		// read from stdin
		while(fgets(buf, 1024, stdin) != NULL){
			printf("%s", buf);
		}
		//printf("Read from stdin (argc = 1)");
		//exit(1);

	}

	// if the first argument is a file, 
	// find option from environment variable and parse from there
	if (argc == 2){
		if (isRegularFile(argv[1]) == 1){
			isFileFirst = 1;
			strcpy(patternStr, getenv("EVERY"));
			fileNamesLen = 1;
			fileNames = malloc(sizeof(char) * fileNamesLen);
			fileNames[0] = argv[1];
		}
		else{
			// read from stdin
			while(fgets(buf, 1024, stdin) != NULL){
				printf("%s", buf);
			}
		}
	}
	
	else{
		// must make sure that all files in list of files are valid
		int i = 0;
		if (isRegularFile(argv[1]) == 1){
			printf("%s\n", "within else");
			i = 1;
			strcpy(patternStr, getenv("EVERY"));
		}
	    else{
			// if the first param is not a str, copy the -N,M arg 
			i = 2;
			strcpy(patternStr, argv[1]);
		}
		int whereStart = i;
		for (; i < argc; i++){
			if (isRegularFile(argv[i]) == -1){
				printf("%s\n", "Invalid File, Usage: every [-N, M] [<list of files>] where M <= N");
				exit(1);
			}
			fileNamesCounter++;
		}
		fileNamesLen = fileNamesCounter;
		printf("In else: fileNamesLen %d\n", fileNamesLen);
		fileNames = malloc(sizeof(char) * fileNamesLen);
		fileNamesCounter = 0;
		// reset i back to original position (whether get argv[1] or argv[2])
		i = whereStart;
		printf("After resetting i should be 2: %d\n", i);
		int amt = fileNamesLen + i;
		for (i; i < amt ; i++){
			printf("within loop: %s\n", argv[i]);	
			fileNames[fileNamesCounter] = argv[i];
			printf("fileNames[%d] : %s\n",fileNamesCounter,  fileNames[fileNamesCounter]);
			fileNamesCounter++;
		}

	}
	// Compile Regular Expression 
	returnVal = regcomp(&regex, pattern2, REG_EXTENDED);
	if (returnVal){
		perror("regcomp");
		exit(1);
	}

	// Execute Regular Expression to find matched groups
	
	if (patternStr != NULL){
		returnVal = regexec(&regex, patternStr, MAX_MATCHES, matches,0);
		if (!returnVal){
			// start matching from group 1
			int i;
			printf("%s\n", "match");
			for (i = 0; i < MAX_MATCHES; i++){
				if (matches[i].rm_so == (size_t)-1){
					break;
				}
				// copy the argv into a string
				char srcCopy[strlen(patternStr) + 1];
				strcpy(srcCopy, patternStr);
				srcCopy[matches[i].rm_eo] = 0;
				printf("Group %u: [%2u-%2u]: %s\n",
					i, matches[i].rm_so, matches[i].rm_eo,
					srcCopy + matches[i].rm_so);
				
				strcpy(matchGroups[i], srcCopy + matches[i].rm_so);	
				//if (i == 1){
				//	strcpy(matchGroupN, srcCopy + matches[i].rm_so);
				//}
				//else if (i == 2){
				//	strcpy(matchGroupM, srcCopy + matches[i].rm_so);
				//}

			}
		}
		else if (returnVal == REG_NOMATCH){
			// if no match, then the first -N, M arg is invalid
			printf("%s\n", "Usage: every [-N, M] [<list of files>] where M <= N");
			exit(1);

		}
		else{
			regerror(returnVal, &regex, errMsg, sizeof(errMsg));
			perror("Regex Matched Failed");
			exit(1);
		}
	}
	// free compiled regular expression
	regfree(&regex);

	// if there is a match, convert the N and M values to integers
	char matchGroupN[128] = "1";
	char matchGroupM[128] = "1";
	int j;
	int numDigits = 0;
	for (j = 0; j < MAX_MATCHES; j++){
		if (isAllDigits(matchGroups[j])){
			numDigits++;
			if (numDigits == 1){
				strcpy(matchGroupN, matchGroups[j]);
			}
			else if (numDigits == 2){
				strcpy(matchGroupM, matchGroups[j]);
			}
			printf("Matched: %s\n", matchGroups[j]);
		}
	}
	int N = atoi(matchGroupN);
	int M = atoi(matchGroupM);
	printf("N: %d\n", N);
	printf("M: %d\n", M);
	if (M < 0 || N < 0){
		printf("Usage: M and N must be positive integers");
		exit(1);
	}
	if (M > N){
		printf("Usage: M must be less than N");
		exit(1);
	}
	
	// BEGIN FILTERING THE FILE
	int i;
	printf("Filename here: %s\n", fileNames[0]);
	printf("filenameslen: %d\n", fileNamesLen);
	for (int i = 0; i < fileNamesLen; i++){
		printf("%d\n", i);
		printf("%s\n", fileNames[i]);
		if ( (fp = fopen(fileNames[i], "r")) == NULL){
				perror("fopen");
				exit(1);
		}
		int currN = 0;
		int currM = 0;
		int currSum = 0;
		while ((fgets(buf, sizeof(buf), fp)) != NULL){
			// print out of every N lines
			if (currN % N == 0){
				// turn the newline [strlen(buf) - 1] into null terminating char 
				buf[strlen(buf) - 1] = '\0';
				printf("%d: %s\n", currN,  buf);
				currSum = currN + M;
			}
			// print out the next M lines
			else if (currN < currSum){
				buf[strlen(buf) - 1] = '\0';
				printf("%d: %s\n", currN,  buf);
			}
			currN++;
		}
		fclose(fp);
	}
	return 0;

}
