// week5_task1_file_io.c
// Name: Kristina  Surname: Deminska Student ID: 251ADB152
//
// Task 1: Read and write data from text files
// Week 5 – Files & Modular Programming
// TODO: Fill in the missing parts marked below.
//
// Build: make lab5   Run: ./bin/week5_task1_file_io


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *filename = "data.txt";
    char line[256];
    int line_count = 0;

    if (argc > 1) {
        filename = argv[1];
    }

    printf("Writing lines to %s...\n", filename);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }

    if (fprintf(fp, "Hello, file I/O in C!\n"
                    "This is another line.\n") < 0) {
        fclose(fp);
        printf("Error writing file.\n");
        return EXIT_FAILURE;
    }

    if (fclose(fp) != 0) {
        perror(filename);
        return EXIT_FAILURE;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return EXIT_FAILURE;
    }

    printf("Reading contents:\n");

    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
        line_count++;
    }

    if (ferror(fp)) {
        fclose(fp);
        printf("Error reading file.\n");
        return EXIT_FAILURE;
    }

    if (fclose(fp) != 0) {
        perror(filename);
        return EXIT_FAILURE;
    }

    printf("Lines read: %d\n", line_count);

    return EXIT_SUCCESS;
}
