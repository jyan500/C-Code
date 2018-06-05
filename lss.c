#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
struct fileInfo{
	mode_t mode;
	nlink_t numLinks;
	uid_t uid;
	gid_t gid;
	off_t size;
	time_t modTime;
	char fileName[128];
	int linkExists;
	char linkName[128];
};
// comparator function for qsort for stat based on size
// pass in two directory entries
int compare(const void *a, const void *b){
	const struct fileInfo * first = (struct fileInfo *) a;
	const struct fileInfo * second = (struct fileInfo *) b;
	return second->size - first->size;

}
int isFileExist(char * path){
	struct stat st;
	int ret;
	if ((ret = stat(path, &st))< 0){
		//perror(path);
	}
	return ret;

}
int isDirectory(char * path){
	struct stat st;
	int ret;
	if ((ret = stat(path, &st)) < 0){
		perror(path);	
	}
	return S_ISDIR(st.st_mode);
}

// takes a struct fi, that contains information about the file
int printFileInfo(struct fileInfo fi, int LFlag){

	struct passwd *pwd;
	struct group *gr;
	char cTime[128];
	if (S_ISLNK(fi.mode)){
		printf("%s", "l");
	}
	else if (S_ISDIR(fi.mode)){
		printf("%s", "d");
	}
	else{
		printf("%s", "-");
	}
	
	// print the permission bits for this file
	printf( (fi.mode & S_IRUSR) ? "r" : "-");
	printf( (fi.mode & S_IWUSR) ? "w" : "-");
	printf( (fi.mode & S_IXUSR) ? "x" : "-");
	printf( (fi.mode & S_IRGRP) ? "r" : "-");
	printf( (fi.mode & S_IWGRP) ? "w" : "-");
	printf( (fi.mode & S_IXGRP) ? "x" : "-");
	printf( (fi.mode & S_IROTH) ? "r" : "-");
	printf( (fi.mode & S_IWOTH) ? "w" : "-");
	printf( (fi.mode & S_IXOTH) ? "x" : "-");
	//
	

	//// print the amount of hard links
	printf(" %d", fi.numLinks);
	// get the username given the uid from stat	
	// print the uid if username not found
	if ( (pwd = getpwuid(fi.uid)) != NULL){
		printf(" %s", pwd->pw_name);
	}
	else{
		perror("getpwuid");
		printf(" %d", fi.uid);
	}

	// get the group name given the gid from stat
	// print the id if name not found
	if ( (gr = getgrgid(fi.gid)) != NULL){
		printf(" %s", gr->gr_name);
	}
	else{
		perror("getgrgid");
		printf(" %d", fi.gid);
	}
	printf(" %d", fi.size);
	strcpy(cTime, ctime(&fi.modTime));
	// process C time
	const char space[2] = " ";
	const char colon[2] = ":";
	char * token = strtok(cTime, space);
	// we want the second, third and fourth token
	// the fourth token we will further divide by colons
	int counter1 = 0;
	int counter2 = 0;

	char month[128];
	char date[128];
	char time[128];
	char hour[6];
	char minute[6];
	while (token != NULL){
		if (counter1 == 1){
			strcpy(month, token);
			
		}
		else if (counter1 == 2){
			 strcpy(date, token);
		}
		else if (counter1 == 3){
			strcpy(time, token);
			char * tok2 = strtok(time, colon);
			while (tok2 != NULL){
				if (counter2 == 0){
					strcpy(hour, tok2);
				}
				else if (counter2 == 1){
					strcpy(minute, tok2);
				}
				tok2 = strtok(NULL, colon);
				++counter2;
			}
		}
		//printf( " %s\n", token);
		
		token = strtok(NULL, space);
		++counter1;
	}

	//printf(" %s", ctime(&fi.modTime));
	printf(" %s %s %s:%s", month, date, hour, minute );
	if (LFlag == 1){
		printf(" %s\n", fi.fileName);
	}
	else{

		if (strcmp(fi.linkName, "") != 0){
			printf(" %s -> %s\n", fi.fileName, fi.linkName);
		}
		else{
			printf(" %s\n", fi.fileName);
		}
	}

	return 0;
}
struct fileInfo returnFileStruct(char * path, int aFlag, int AFlag, int LFlag){
	struct stat st;
	struct fileInfo fi;
	size_t lenLink;
	char linkname[128];
	if (LFlag == 1){
	
		if ((stat(path, &st)) < 0){
			perror("stat");
		}
	}
	else{

		if ((lstat(path, &st)) < 0){
			perror("lstat");
		}
	}	
	// if there's a softlink, get the name of it
	if ( (lenLink = readlink(path, linkname, sizeof(linkname) - 1)) != -1){
		
		linkname[lenLink] = '\0';
		strcpy(fi.linkName, linkname);
		if (isFileExist(linkname) == -1){
			//printf("linkname doesnt exist: %s\n", linkname);
			perror(path);
			fi.linkExists = -1;
		}
		else{
			//printf("linkname: %s\n", linkname);
			fi.linkExists = 0;
		}
	}
	//if there's no symbolic link, define as empty string 
	else{
		strcpy(fi.linkName, "");
	}
	fi.mode = st.st_mode;
	fi.numLinks = st.st_nlink;
	fi.uid = st.st_uid;
	fi.gid = st.st_gid;
	fi.size = st.st_size;
	fi.modTime = st.st_mtime;
	strcpy(fi.fileName,path );

	return fi;
}
int printStatDirectory(char * path, int aFlag, int AFlag, int LFlag){
	DIR *d;
	struct dirent *dir;
	struct stat st;
	struct stat linkStat;
	struct passwd *pwd;
	struct group *gr;
	size_t lenLink;
	char cTime[128];
	char linkname[128];
	d = opendir(path);
	
	int lenOfFileArray = 0;
	if (d){
		while ( (dir = readdir(d)) != NULL){
			// if the aFlag is not set, don't include files that begin with .
			if (dir->d_name[0] == '.'){
				if (AFlag == 1){
					//printf("Is this true %d\n", dir->d_name == ".");
					if (strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name, "..") == 0){
						continue;
					}
				}
				else if (aFlag != 1){
					continue;
				}
				
			}
			//printf("while getting lenOfaRray %s\n", dir->d_name);
			++lenOfFileArray;
			
		}
		if (closedir(d) < 0){
			perror("closedir");
		}
	}
	else{
		perror("opendir");
		return -1;
	}
	struct fileInfo* fileArray = malloc(lenOfFileArray * sizeof(*fileArray));
	if (fileArray == NULL){
		perror("malloc");
	}
	d = opendir(path);
	int i = 0;

	//printf("lenOfFileArray: %d\n", lenOfFileArray);
	if (d){
		while ( (dir = readdir(d)) != NULL){
			if (!dir){
				perror("readdir");
				exit(-1);
			}

			// include the path as part of the filename
			// when putting into struct, don't include the path
			char tempFileName[128];
			strcpy(tempFileName, path);
			strcat(tempFileName, "/");
			strcat(tempFileName, dir->d_name);
			//printf("Dirname: %s\n", tempFileName);
			// if LFlag is true, ignore symbolic links
			if (LFlag == 1){
				
				if ( (stat(tempFileName, &st) < 0)){
					//printf("Dirname that's causing error: %s\n", dir->d_name);
					//perror("stat");
				}

			}
			else{
				if ( (lstat(tempFileName, &st) < 0)){
					perror("lstat");
				}
			}
			// if file is a . or .. and AFlag is not set, don't include
			struct fileInfo ft;
			//printf("Current one before crashes: %s\n", dir->d_name);
			//printf("current index: %d\n", i);
			// if file starts with a . and aflag is not set, don't include
			if (dir->d_name[0] == '.'){
				if (AFlag == 1){
					if (strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name, "..") == 0){
						continue;
					}

				}
				else if (aFlag != 1){
					continue;
				}
				
			}
			//printf("Crashing 3? on %s\n", dir->d_name);
			fileArray[i].mode = st.st_mode;
			fileArray[i].numLinks = st.st_nlink;
			fileArray[i].uid = st.st_uid;
			fileArray[i].gid = st.st_gid;
			fileArray[i].size = st.st_size;
			fileArray[i].modTime = st.st_mtime;
			strcpy(fileArray[i].fileName ,dir->d_name);	
			// if there's a softlink, get the name of it
			if ( (lenLink = readlink(tempFileName, linkname, sizeof(linkname) - 1)) != -1){
				
				linkname[lenLink] = '\0';
				strcpy(fileArray[i].linkName, linkname);
				if (isFileExist(linkname) == -1){
					//printf("linkname doesnt exist: %s\n", linkname);
					perror(tempFileName);
					fileArray[i].linkExists = -1;
				}
				else{
					//printf("linkname: %s\n", linkname);
					fileArray[i].linkExists = 0;
				}
			}
			//if there's no symbolic link, just copy the filename into the linkname (since its a hardlink)
			else{
				strcpy(fileArray[i].linkName, "");
			}

			i++;
		}
		if (closedir(d) < 0){
			perror("closedir");
			exit(-1);
		}
	}
	else{
		perror("opendir");
		return -1;
	}
	qsort(fileArray, lenOfFileArray, sizeof(*fileArray), compare);
	int printLater = 0;
	struct fileInfo sav;
	int j = 0;
	for (j = 0; j < lenOfFileArray; j++){
		struct fileInfo fi = fileArray[j];
		// if the link doesn't exist and using -L option, print this out as the last link
		if (fi.linkExists == -1 && LFlag == 1){
			printLater = 1;
			memcpy(&sav, &fi, sizeof(fi));
			continue;
		}
		printFileInfo(fi, LFlag);
	}
	if (printLater && LFlag == 1){
		printf("l????????? ?     ?      ?        ?    ");
		printf("%s\n", sav.fileName);
	}
	free(fileArray);
	return 0;
	
}
int main(int argc, char * argv[]){
	int aFlag=0;
	int AFlag=0;
	int LFlag=0;
	int opt;
	int err = 0;
	extern int optind;

	// parse the optional arguments
	while ((opt = getopt(argc, argv, "aAL")) != -1){
		switch (opt){
			case 'a':
				aFlag = 1;
				break;
			case 'A':
				AFlag = 1;
				break;
			case 'L':
				LFlag = 1;
				break;
			case '?':
				// if an invalid option is given, exit
				err = 1;
				exit(1);
		}
	}
	//printf("%d %d %d\n", aFlag, AFlag, LFlag);
	// check for filenames as arguments
	
	int optIndCopy = optind;
	int numFiles = 0;
	if (optind < argc){
		for (; optind < argc; optind++){
			if (isDirectory(argv[optind]) == 1){
				//printf("argument: %s is a directory\n", argv[optind]);
				printf("%s: \n", argv[optind]);
				printStatDirectory(argv[optind], aFlag, AFlag, LFlag);
				printf("\n");
			}
			else if (isFileExist(argv[optind]) == 0){
				numFiles++;
				//printf("argument: %s is a file\n", argv[optind]);
			}
		}
	}
	// no additional arguments such as directories or filenames are given, assume the current working directory
	else{
		//printf("no arguments left to process\n");
		printStatDirectory(".", aFlag, AFlag, LFlag);
	}
	// create fileArray for multiple files in the command line, sort them and print out the stats of each	
	if (numFiles != 0){
		int i = 0;
		struct fileInfo * fileArray = malloc(numFiles * sizeof(*fileArray));
		if (fileArray == NULL){
			perror("malloc");
		}
		for (; optIndCopy < argc; optIndCopy++){
			if (isFileExist(argv[optIndCopy]) == 0){
				struct fileInfo ft = returnFileStruct(argv[optIndCopy], aFlag, AFlag, LFlag);
				memcpy(&fileArray[i], &ft, sizeof(ft));
				i++;
			}
		}
		qsort(fileArray, numFiles, sizeof(*fileArray), compare);
		int j = 0;
		for (j = 0; j < numFiles; j++){
			struct fileInfo fi = fileArray[j];
			printFileInfo(fi, LFlag);
		}
		
	
	}
	
	return 0;
}
