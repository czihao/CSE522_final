/*
  Program that allocates a block of memory and writes
  data to it at random index locations. Takes an offset
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "paging.h"

#define NUM_EXPECTED_ARGS 4 

static void usage(void)
{
    printf("Usage: ./random_access <size> <loops> <mmu mode>\n");
}

int main(int argc, char ** argv)
{
    int i = 0, fd;
    int ret;
	unsigned int step = 0;
    unsigned long int map_size;
    int use_mmap_driver = 0;
    int *address = NULL;
    int *orig_addr = NULL;
    unsigned long int loops;
    struct timespec start_time;
    struct timespec end_time;

    if (argc != NUM_EXPECTED_ARGS)
    {
      printf("Incorrect number of arguments\n");
      usage();
      exit(EXIT_FAILURE);
    }

    sscanf(argv[1], "%lu", &map_size);
    sscanf(argv[2], "%lu", &loops);
    use_mmap_driver = atoi(argv[3]);
    
    if (map_size % 2 != 0)
    {
        printf("Size must be even\n");
        usage();
        exit(EXIT_FAILURE);
    }
    
    if (loops < 0)
    {
        printf("Loops must be greater than 0\n");
        usage();
        exit(EXIT_FAILURE);
    }

    get_time(&start_time);

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }
    if (use_mmap_driver)
	{
		address = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (address == MAP_FAILED) {
		    fprintf(stderr, "Could not mmap " DEV_NAME ": %s\n", strerror(errno));
		    return NULL;
		}
    }
    else {
    	address = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    } 



    unsigned long int num_map_entries = map_size / sizeof(int);
    int rand_index;
    
    for (int i=0; i<loops; i++)
    {
        rand_index = get_rand(0, num_map_entries-1);
        address[rand_index] = i;
    }
    
    munmap(orig_addr, map_size);
    get_time(&end_time);
    printf("Execution time: %.9f secs\n", timediff(start_time, end_time));
    
    return ret;   
}
