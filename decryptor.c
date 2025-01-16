/*
* Program to decrypt encrypted message with given key
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void decrypt(const char *input_file, const char *key) {
    //File status structure
    struct stat file_stat;
    //stat system call return information about a file to stat structure
    if (stat(input_file, &file_stat) < 0) {
        perror("Error getting file status");
        exit(EXIT_FAILURE);
    }
    //st_mode includes information about file type and mode.
    //Macro S_ISCHAR() checks if the file is character device.
    if (!S_ISCHR(file_stat.st_mode)) {
        fprintf(stderr, "Error: %s is not a character device\n", input_file);
        exit(EXIT_FAILURE);
    }
    //Open binary file in read mode
    FILE *file = fopen(input_file, "rb");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    size_t key_len = strlen(key);
    size_t key_index = 0;
    int ch;

    //fgetc() reads the next character until EOF occurs from file stream.
    while ((ch = fgetc(file)) != EOF) {
        ch ^= key[key_index % key_len];
        //print character to stdout
        putchar(ch);
        key_index++;
    }

    //Close the file
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input file> <key>\n", argv[0]);
        return EXIT_FAILURE;
    }
    decrypt(argv[1], argv[2]);
    return EXIT_SUCCESS;
}