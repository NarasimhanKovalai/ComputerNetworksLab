
/*Name-Narasimhan Kovalai
Roll Number-20CS01075
Assignment 2 Error Detection
Computer Network Lab*/

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include<signal.h>

/*64 bytes=512 bits and 4 bytes=32 bits*/
#define payloadsize 512
#define checksumsize 32

int modified_payload_copy[payloadsize + checksumsize];
int modified_payload_read[payloadsize + checksumsize];

void random_generator(int *arr)
{

    int i = 0;
    for (i = 0; i < payloadsize; i++)
    {
        arr[i] = rand() % 2;
    }
}

void introduce_error()
{
    /*choose either 1 or 2 bits to be flipped*/
    int x = 1 + rand() % 2;

    while (x--)
    {
        /*choose which index(or indices) to be flipped*/
        int y = rand() % payloadsize;
        modified_payload_copy[y] = 1 - modified_payload_copy[y];
    }
}

void perform_checksum(int *arr, int *checksum, int x, int y)
{
    /*taking XOR followed by 1's complement */
    int n = x / y;
    int table[n][y];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < y; j++)
            table[i][j] = arr[i * (y) + j];

    for (int i = 1; i < n; i++)
        for (int j = 0; j < y; j++)
            table[i][j] ^= table[i - 1][j];

    for (int i = 0; i < y; i++)
        checksum[i] = table[n - 1][i] ^ 1;
}

int see_if_error(int *checksums)
{
    for (int i = 0; i < checksumsize; i++)
    {
        if (checksums[i] != 0)
        {
            return 1;
        }
    }
    return 0;
}
int main(int argc, char *argv[])
{

    // payload array of 64 bytes
    float chance = (float)atoi(argv[1]) / 100;
    // printf("Error probability is =%f\n", chance);
    /*seed to generate unique values each time*/
    srand(time(0));
    int arr[payloadsize];
    random_generator(arr);
    // printf("Original message is : \n");

    // for (int i = 0; i < payloadsize; i++)
    // {
    //     printf("%d", arr[i]);
    // }
    // printf("\n");
    // printf("\n Checksum is :\n");

    int checksum[checksumsize] = {0};
    int r;

    perform_checksum(arr, checksum, payloadsize, checksumsize);

    // for (int h = 0; h < checksumsize; h++)
    // {
    //     printf("%d", checksum[h]);
    // }

    int modified_payload[payloadsize + checksumsize];
    int i = 0;
    for (int i = 0; i < payloadsize + checksumsize; i++)
    {
        if (i < payloadsize)
        {
            modified_payload[i] = arr[i];
        }
        else
        {
            modified_payload[i] = checksum[i - payloadsize];
        }
    }

    // printf("\nexpanded\n");
    // for (int h = 0; h < checksumsize + payloadsize; h++)
    // {
    //     printf("%d", modified_payload[h]);
    // }
    introduce_error();

    // printf("\nreceiver check sum is :\n");

    int checksums[checksumsize] = {7};
    perform_checksum(modified_payload_copy, checksums, payloadsize + checksumsize, checksumsize);
    // for (int h = 0; h < checksumsize; h++)
    // {
    //     printf("%d", checksums[h]);
    // }

    /*declaring file descriptor for pipe communication*/

    int file_des[2];
    if (pipe(file_des) == -1)
    {
        fprintf(stderr, "Error in pipe() system call");
        return 1;
    }
    /*Flush the buffer before proceeding with fork*/
    fflush(0);
    int parent = getpid();
    int c = fork();
    if (c < 0)
    {
        fprintf(stderr, "Forking failed to create child process");
        return 1;
    }
    else if (c == 0)
    {
        close(file_des[1]); // prevents program from hanging
        int callbacks = 0;
        while (2 > 1)
        {
            /*store the read result into modified_payload_read, We are using different
            arrays to avoid bufer overwriting. */
            read(file_des[0], modified_payload_read, sizeof modified_payload_read);
            callbacks += 1;
            perform_checksum(modified_payload_read, checksums, payloadsize + checksumsize, checksumsize);
            if (!see_if_error(checksums))
            {
                break;
            }
        }
        /*kill parent process after termination*/
        kill(parent, SIGKILL);
        /*prints number of callbacks made before succesfull transmission*/
        printf("%d\n", callbacks);
    }
    else
    {
        close(file_des[0]); // prevents hanging of program
        while (2 > 1)
        {
            for (int i = 0; i < payloadsize + checksumsize; i++)
            {
                modified_payload_copy[i] = modified_payload[i];
            }
            /*generating a probability with 2 decimal places*/
            float gen = (float)(rand() % 100) / 100;
            if (gen <= chance)
            {
                introduce_error();
            }

            write(file_des[1], modified_payload_copy, sizeof modified_payload_copy);
        }
    }
    return 0;
}
