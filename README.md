# File-organiser-on-Linux

System programming project. We noticed the problem with file organisation on our VM machines. There are a lot of files in different places. And we don’t  have a time to organise it.
That’s why we want to make a program which handle this disorganisation of files.

## Usage

To build the files:

``` bash
make all
```

To test the program:

``` bash
make test
```

To remove the files created by the previous commands:

``` bash
make clean
```

## Assumptions

Program has following options:

- Delete all children directories and put all files in one place to be able to organise them properly

- Assign all files to directories by date of  creation, e.g. folder 2018, 2019 each of them consisting of directories with months

- Assign all files to directories by type of file ( .c, .txt, .mp3)

- Assign all files to directories by alphabetical order ( directories A-J, K-R, S-Z)

- Assign all files to directories by type of group or owner

## Stages of project creation

- creating structures to keep all the files

- implementing directories deleting option

- Implementing files assigning by different criteria

- testing
