/*
 * Goal : move the files contained in subdirectories in the working directory
 *
 *  Go to each subdirectory, check if it has subdirectories
 *  if yes : go to each subdirectory and restart (recursive)
 *  if no : move the files to the initial directory
 *
 * Functions needed :   check every file of the current directory
 *          if it is a directory (but not . or ..), go in it and restart
 *          if not move the files to the initial directory
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

void rebDirCont(char []);
int checkDir(char*);
ino_t get_inode(char*);
void getPathTo(ino_t);
void getPathToS(ino_t);
void inum_to_name(ino_t, char*, int);

char initial_path[256];
char temp_path[256];
char oldpath[256];
char newpath[256];

int main(int argc, char *argv[]) {
    // give the value of the actual path to the value initial_path
    getPathTo( get_inode(".") );
    //printf("Initial path : %s\n", initial_path);
    rebDirCont(".");
    return 0;
}

void rebDirCont(char dirname[]) {
    DIR* dir_ptr;
    struct dirent* direntp;
    static int deepness=0;

    // if( (dir_ptr = opendir("/home/jean/test"/*dirname*/)) == NULL)
    if ((dir_ptr = opendir(dirname)) == NULL) {
        perror(dirname);
        exit(1);
    }
    else {
        chdir(dirname);
        // While there are remaining files
        while( (direntp = readdir(dir_ptr)) != NULL) {
            // Check if the file is a directory, if it is a directory, then rebDirCont on it
            if (checkDir(direntp->d_name) == 1 && strcmp(direntp->d_name, "..") != 0 &&
                strcmp(direntp->d_name, ".") != 0) {
                //printf("%s\n", direntp->d_name);
                deepness++;
                rebDirCont(direntp->d_name);
            }
            else if (strcmp(direntp->d_name, "..") != 0 &&
                     strcmp(direntp->d_name, ".") != 0) {
                // rename the file with initial path + filename, use the pwd function that we made on week 4
                // if( strcmp(temp_path, initial_path) == 0)
                if (deepness == 0) {
                    continue;
                }
                printf("%s\n", direntp->d_name);
                getPathToS( get_inode(".") );
                printf("%s\n", temp_path);
                //rename :
                bzero(oldpath, strlen(oldpath));
                bzero(newpath, strlen(newpath));
                strcat(oldpath, temp_path);
                strcat(oldpath, "/");
                strcat(oldpath, direntp->d_name);
                strcat(newpath, initial_path);
                strcat(newpath, "/");
                strcat(newpath, direntp->d_name);
                printf("Oldpath : %s\nNewpath : %s\n", oldpath, newpath);
                if(rename(oldpath, newpath) == -1)
                    perror("rename");
            }
        }
        deepness--;
        closedir(dir_ptr);
        chdir("..");
    }
}

// Function testing if the file is a directory, return 1 if yes, 0 otherwise
int checkDir(char filename[]) {
    struct stat info;

    if (stat(filename, &info) == -1) {
        perror(filename);
    }
    else {
        if (S_ISDIR(info.st_mode)) {            // If the file is a directory : relaunch
            return 1;
        }
    }
    return 0;
}


/*
int main()
{
    if(get_inode(".")==get_inode("/"))
        printf("/");
    else
        printpathto( get_inode( "." )); 	// Print path to here
    putchar('\n');				// Then add new line
    return 0;
}
*/

// Returns inode number of the file
ino_t get_inode(char *fname) {
    struct stat info;

    if (stat( fname, &info ) == -1) {
        fprintf(stderr, "Cannot stat ");
        perror(fname);
        exit(1);
    }
    return info.st_ino;
}

// Prints path leading down to an object with this inode kindof recursively
void getPathTo(ino_t this_inode) {
    ino_t my_inode;
    char its_name[BUFSIZ];

    bzero(initial_path, strlen(initial_path));
    if (get_inode("..") != this_inode) {
        chdir("..");                            // Up one dir
        inum_to_name(this_inode, its_name, BUFSIZ);     // Get its name
        my_inode = get_inode( "." );            // Print head
        getPathTo( my_inode );                  // Recursively
        //printf("/%s", its_name);              // now print name of this
        strcat(initial_path, "/");
        strcat(initial_path, its_name);
        chdir(its_name);
    }
}

// Prints path leading down to an object with this inode kindof recursively
void getPathToS(ino_t this_inode) {
    ino_t my_inode;
    //static int i=0;
    char its_name[BUFSIZ];

    bzero(temp_path, strlen(temp_path));
    if (get_inode("..") != this_inode) {
        chdir("..");                            // Up one dir
        inum_to_name(this_inode, its_name, BUFSIZ);     // Get its name
        my_inode = get_inode( "." );            // Print head
        getPathToS( my_inode );                 // Recursively
        //printf("/%s", its_name);              // now print name of this
        strcat(temp_path, "/");
        strcat(temp_path, its_name);
        chdir(its_name);
        //printf("%d\n", i++);
    }
}

// Looks through current directory for a file with this inode number
// and copies its name into namebuf
void inum_to_name(ino_t inode_to_find, char *namebuf, int buflen) {
    DIR *dir_ptr;                               // The directory
    struct dirent *direntp;                     // Each entry

    dir_ptr = opendir( "." );
    if (dir_ptr == NULL) {
        perror( "." );
        exit(1);
    }
    // Search directory for a file with specified inum
    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (direntp->d_ino == inode_to_find) {
            strncpy(namebuf, direntp->d_name, buflen);
            namebuf[buflen-1] = '\0';           // Just in case
            closedir(dir_ptr);
            return;
        }
    }
    fprintf(stderr, "error looking for inum %lu\n", inode_to_find);
    exit(1);
}
