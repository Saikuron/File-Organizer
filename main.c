///////////////////////////////////////////////
//  GROUPING
//      Concept:
//  1. Create array of srtuctures to list all files
//  2. Traverse through them and check their type, date or first letter
//  3. Create new directory with type date or first letter name(if doesn't exist)
//  4. Move file to directory
//  Warning! Do not move:  .  ..  PROGRAMNAME   PROGRAMNAMEC
//////////////////////////////////////////////

/* MOVING ALL TO ONE PLACE
 * Goal : move the files contained in subdirectories in the working directory
 *
 *  Go to each subdirectory, check if it has subdirectories
 *  if yes : go to each subdirectory and restart
 *  if no : move the files to the initial directory
 *
 * Functions needed : check every file of the current directory
 *          if it is a directory (but not . or ..), go in it and restart
 *          if not move the files to the initial directory
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

// for deletion:
void rebDirCont(char[]);
int checkDir(char*);
void getPathTo(ino_t);
void getPathToS(ino_t);
ino_t get_inode(char*);
void inum_to_name(ino_t, char*, int);

char initial_path[256];
char temp_path[256];
char oldpath[256];
char newpath[256];

// for grouping
void do_struct(char[]);
void print_dirent(struct dirent*);
void dostat(char*, int i);
void get_file_info(char*, struct stat*, int i);
void mode_to_letters(int, char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);
void do_struct_sort(char[], bool reverse);
void type_group(const char* s2);
void date_group(const char* s2);
void alpha_group(const char* s2);
char* merge_path(const char* s1, const char* s2);
void move_file(char* source, const char* dest, const char* s2);

#define SIZE 70
#define PROGRAMNAME "main"
#define PROGRAMNAMEC "main.c"

struct filestruct {
    char mode[11];
    long int ino;
    int st_nlink;
    char uid[8];
    long int st_size;
    char time[30];
    char filename[1024];
};

struct filestruct d1[SIZE];
struct stat st = {0};


int main(int ac, char* argv[]) {
    char pt[PATH_MAX];
    int i = 1, j = 0;
    int op_alpha = 0, op_type = 0, op_date = 0, op_rmdir = 0;

    for (i = 1; i < ac; i++) {
        if (strncmp(argv[i], "-", strlen("-")) == 0) {
            for (j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                    case 'a':
                        op_alpha = 1;
                        break;
                    case 't':
                        op_type = 1;
                        break;
                    case 'd':
                        op_date = 1;
                        break;
                    case 'D':
                        op_rmdir = 1;
                        break;
                    default:
                        printf("%c is not a valid option\n", argv[i][j]);
                        exit(1);
                        break;
                }
            }
        }
    }

    if (op_alpha + op_type + op_date > 1) {
        fprintf(stderr, "Error with the options, you have to choose only one "
                        "among -a -t and -d");
        exit(1);
    }

    // Get path to current working directory
    if (getcwd(pt, sizeof(pt)) == NULL) {
        perror("Error: getcwd() - Couldn't get current working directory");
    }
    do_struct(".");

    // arguments' options
    if (op_rmdir == 1) {
        printf("Deletion of directories\n");
        getPathTo(get_inode("."));
        //printf("Initial path : %s\n", initial_path);
        rebDirCont(".");
    }
    if (op_type == 1) {
        printf("\nType group option \n");
        type_group(pt);
    }
    else if (op_date == 1) {
        printf("\nDate group option \n");
        date_group(pt);
    }
    else if (op_alpha == 1) {
        printf("\nAlphabetically group option \n");
        alpha_group(pt);
    }
    return 0;
}

void rebDirCont(char dirname[]) {
    DIR* dir_ptr;
    struct dirent* direntp;
    static int deepness = 0;

    //if( (dir_ptr = opendir("/home/jean/test"/*dirname*/)) == NULL)
    if ((dir_ptr = opendir(dirname)) == NULL) {
        perror(dirname);
        exit(1);
    }
    else {
        chdir(dirname);
        // While there are remaining files
        while ((direntp = readdir(dir_ptr)) != NULL) {  
            // Check if the file is a directory, if it is a directory, then 
            // rebDirCont on it
            if (checkDir(direntp->d_name) == 1 &&
                strcmp(direntp->d_name, "..") != 0 &&
                strcmp(direntp->d_name, ".") != 0) {
                //printf("%s\n", direntp->d_name);
                deepness++;
                rebDirCont(direntp->d_name);
                //printf("Suppress : %s\n", direntp->d_name);
                rmdir(direntp->d_name);
            }
            else if (strcmp(direntp->d_name, "..") != 0 &&
                     strcmp(direntp->d_name, ".") != 0) {
                // rename the file with initial path + filename, use the pwd 
                // function that we made on week 4
                //if( strcmp(temp_path, initial_path) == 0)
                if (deepness == 0) {
                    continue;
                }
                //printf("file to be moved : %s\n", direntp->d_name);
                getPathToS(get_inode("."));
                //printf("%s\n", temp_path);
                //rename :
                bzero(oldpath, strlen(oldpath));
                bzero(newpath, strlen(newpath));
                strcat(oldpath, temp_path);
                strcat(oldpath, "/");
                strcat(oldpath, direntp->d_name);
                strcat(newpath, initial_path);
                strcat(newpath, "/");
                strcat(newpath, direntp->d_name);
                //printf("Oldpath : %s\nNewpath : %s\n", oldpath, newpath);
                if (rename(oldpath, newpath) == -1) {
                    perror("rename");
                }
            }
        }
        deepness--;
        closedir(dir_ptr);
        chdir("..");
    }
}

// Function that test if the file is a directory, return 1 if yes, 0 otherwise
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
ino_t get_inode(char* fname) {
    struct stat info;

    if (stat(fname, &info) == -1) {
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
        my_inode = get_inode(".");              // Print head
        //printf("%s", its_name);               // now print name of this
        getPathTo(my_inode);                    // Recursively
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
        my_inode = get_inode(".");              // Print head
        getPathToS(my_inode);                   // Recursively
        //printf("/%s", its_name);              // now print name of this
        strcat(temp_path, "/");
        strcat(temp_path, its_name);
        chdir(its_name);
        //printf("%d\n", i++);
    }
}

// Looks through current directory for a file with this inode number
// and copies its name into namebuf
void inum_to_name(ino_t inode_to_find, char* namebuf, int buflen) {
    DIR* dir_ptr;                               // The directory
    struct dirent* direntp;                     // Each entry

    dir_ptr = opendir(".");
    if (dir_ptr == NULL) {
        perror(".");
        exit(1);
    }
    // Search directory for a file with specified inum
    while ((direntp = readdir(dir_ptr)) != NULL) {
        if (direntp->d_ino == inode_to_find) {
            strncpy(namebuf, direntp->d_name, buflen);
            namebuf[buflen - 1] = '\0';         // Just in case
            closedir(dir_ptr);
            return;
        }
    }
    fprintf(stderr, "error looking for inum %lu\n", inode_to_find);
    exit(1);
}

///////////////////////////////GROUPING FUNCTIONS: //////////////////

void alpha_group(const char* s2) {
    //protect OUR program, don't move yourself!
    int z, i, k, d;
    char type[SIZE][10];

    for (z = 0; z < SIZE; z++) {
        if (d1[z].ino == 0) {
            break;
        }
        if ((strcmp(d1[z].filename, "..") == 0) ||
            (strcmp(d1[z].filename, ".") == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAME) == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAMEC) == 0)) {
            //don't move file
        }
        else {
            type[z][0] = '/';
            type[z][1] = toupper(d1[z].filename[0]);
            type[z][2] = '\0';
            //create directories
            const char* path = merge_path(s2, type[z]);
            if (stat(path, &st) == -1) {
                mkdir(path, 0700);
            }
            //move file
            char* r_name = merge_path(s2, "/");
            char* o_name = merge_path(r_name, d1[z].filename);
            char* new_name = merge_path(path, "/");
            char* new_name2 = merge_path(new_name, d1[z].filename);
            //printf("MOVE! old name = %s, new name = %s\n", o_name, new_name2);

            if (rename(o_name, new_name2) == 0) {
                printf("File renamed successfully.\n");
            }
            else {
                printf("Unable to rename files. Please check files exist and "
                       "you have permissions to modify files.\n");
            }
        }
    }
}


void date_group(const char* s2) {
    //protect OUR program, don't move yourself!
    int z, i, k, d;
    char mnth[SIZE][6];
    char year[SIZE][6];

    for (z = 0; z < SIZE; z++) {
        if (d1[z].ino == 0) {
            break;
        }
        if ((strcmp(d1[z].filename, "..") == 0) ||
            (strcmp(d1[z].filename, ".") == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAME) == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAMEC) == 0)) {
            //don't move file
        }
        else {
            //get Year
            year[z][0] = '/';
            year[z][1] = d1[z].time[20];
            year[z][2] = d1[z].time[21];
            year[z][3] = d1[z].time[22];
            year[z][4] = d1[z].time[23];
            year[z][5] = '\0';
            //printf("YEAR = %s \n", year[z]);
            //create Directory
            const char* path = merge_path(s2, year[z]);
            if (stat(path, &st) == -1) {
                mkdir(path, 0700);
            }
            mnth[z][0] = '/';
            mnth[z][1] = d1[z].time[4];
            mnth[z][2] = d1[z].time[5];
            mnth[z][3] = d1[z].time[6];
            mnth[z][4] = '\0';
            const char* path2 = merge_path(path, mnth[z]);
            if (stat(path2, &st) == -1) {
                mkdir(path2, 0700);
            }
            //move file
            char* r_name = merge_path(s2, "/");
            char* o_name = merge_path(r_name, d1[z].filename);
            char* new_name = merge_path(path2, "/");
            char* new_name2 = merge_path(new_name, d1[z].filename);
            //printf("MOVE! old name = %s, new name = %s\n", o_name, new_name2);
            if (rename(o_name, new_name2) == 0) {
                printf("File renamed successfully.\n");
            }
            else {
                printf("Unable to rename files. Please check files exist and "
                       "you have permissions to modify files.\n");
            }
        }//end else
    }
}


void type_group(const char* s2) {
    //protect OUR program, don't move yourself!
    int z, i, k, d;
    char type[SIZE][10];

    for (z = 0; z < SIZE; z++) {
        if (d1[z].ino == 0) {
            break;
        }
        if ((strcmp(d1[z].filename, "..") == 0) ||
            (strcmp(d1[z].filename, ".") == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAME) == 0) ||
            (strcmp(d1[z].filename, PROGRAMNAMEC) == 0)) {
            //don't move file
        }
        else {
            for (i = 0; d1[z].filename[i] != '\0'; i++) {
                if (d1[z].filename[i] == '.') {
                    d = 0;
                    type[z][d] = '/';
                    d++;
                    for (k = i + 1; d1[z].filename[k] != '\0'; ++k) {
                        type[z][d] = d1[z].filename[k];
                        d++;
                    }
                    type[z][d] = '\0';
                }
            }
            if (type[z][0] == '/') {
                //if type of file was given after dot in filename field
                //create directories
                const char* path = merge_path(s2, type[z]);
                if (stat(path, &st) == -1) {
                    mkdir(path, 0700);
                }
                //move file
                char* r_name = merge_path(s2, "/");
                char* o_name = merge_path(r_name, d1[z].filename);
                char* new_name = merge_path(path, "/");
                char* new_name2 = merge_path(new_name, d1[z].filename);
                //printf("MOVE! old name = %s, new name = %s\n", o_name, new_name2);
                if (rename(o_name, new_name2) == 0) {
                    printf("File renamed successfully.\n");
                }
                else {
                    printf("Unable to rename files. Please check files exist and "
                           "you have permissions to modify files.\n");
                }
            }
            else {
                //create directory for programs
                const char* path = merge_path(s2, "/programs");
                if (stat(path, &st) == -1) {
                    printf("error");
                    mkdir(path, 0700);
                }
                //move file
                char* r_name = merge_path(s2, "/");
                char* o_name = merge_path(r_name, d1[z].filename);
                char* new_name = merge_path(path, "/");
                char* new_name2 = merge_path(new_name, d1[z].filename);
                //printf("MOVE! old name = %s, new name = %s\n", o_name, new_name2);
                if (rename(o_name, new_name2) == 0) {
                    printf("File renamed successfully.\n");
                }
                else {
                    printf("Unable to rename files. Please check files exist and "
                           "you have permissions to modify files.\n");
                }
            }
        } //end else
    }
}


char* merge_path(const char* s1, const char* s2) {
    char* result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


void do_struct(char dirname[]) {
    DIR* dir_ptr;
    struct dirent* direntp;
    int i = 0;

    // Open directory with name dirname
    if ((dir_ptr = opendir(dirname)) == NULL) {
        fprintf(stderr, "ls1: cannot open %s\n", dirname);
    }
    else {
        // If dirname is not current directory, cd to it
        if (strcmp(dirname, ".") != 0) {
            chdir(dirname);
        }
        // Go through each entry of the directory
        while ((direntp = readdir(dir_ptr)) != NULL) {
            // print_dirent(direntp);              // Debug
            dostat(direntp->d_name, i);
            i++;
        }
        closedir(dir_ptr);
        // Go back to original directory if cd was done
        if (strcmp(dirname, ".") != 0) {
            chdir("..");
        }
    }
}


// Debug function to print the content of a dirent struct
void print_dirent(struct dirent* direntp) {
    printf("%s - %d - %d\n", direntp->d_name, direntp->d_ino, direntp->d_reclen);
}


void dostat(char* filename, int i) {
    struct stat info;

    if (stat(filename, &info) == -1) {
        perror(filename);
    }
    else {
        get_file_info(filename, &info, i);
    }
}


// Copies file information to d1
void get_file_info(char* filename, struct stat* info_p, int i) {
    char modestr[11];

    mode_to_letters(info_p->st_mode, modestr);
    strcpy(d1[i].mode, modestr);
    d1[i].ino = info_p->st_ino;
    d1[i].st_nlink = (int) info_p->st_nlink;
    strcpy(d1[i].uid, uid_to_name(info_p->st_uid));
    d1[i].st_size = (long) info_p->st_size;
    strcpy(d1[i].time, ctime(&info_p->st_mtime));
    strcpy(d1[i].filename, filename);
}


void mode_to_letters(int mode, char str[]) {
    strcpy(str, "----------");
    if (S_ISDIR(mode)) str[0] = 'd';
    if (S_ISCHR(mode)) str[0] = 'c';
    if (S_ISBLK(mode)) str[0] = 'b';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}


// Get user id and return it
char* uid_to_name(uid_t uid) {
    struct passwd* getpwuid(), * pw_ptr;
    static char numstr[10];

    // Get user id
    if ((pw_ptr = getpwuid(uid)) == NULL) {
        sprintf(numstr, "%d", uid);
        return numstr;
    }
    else {
        return pw_ptr->pw_name;
    }
}


// Get group id and return it
char* gid_to_name(gid_t gid) {
    struct group* getgrid(), * grp_ptr;
    static char numstr[10];

    if ((grp_ptr = getgrgid(gid)) == NULL) {
        sprintf(numstr, "%d", gid);
        return numstr;
    }
    else {
        return grp_ptr->gr_name;
    }
}
