/*
  Simple program that allocates a block of memory and writes
  data to it in sequential order.
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
    printf("Usage: ./simple_sequential <size> <mmap function>\n");
}

// Returns the size of the L3 cache in bytes, otherwise -1 on error.
int cache_size(int arg) {
    // We grab the cache size from cpu 0, assuming that all cpu caches are the
    // same size.
    const char* cache_size_path =
        "/sys/devices/system/cpu/cpu0/cache/index3/size";

	FILE *cache_size_fd;
	if (!(cache_size_fd = fopen(cache_size_path, "r"))) {
		perror("could not open cache size file");
		return -1;
	}

	char line[512];
	if(!fgets(line, 512, cache_size_fd)) {
		fclose(cache_size_fd);
		perror("could not read from cache size file");
		return -1;
	}

	// Strip newline
	const int newline_pos = strlen(line) - 1;
	if (line[newline_pos] == '\n') {
		line[newline_pos] = '\0';
	}

	// Get multiplier
	int multiplier = 1;
	const int multiplier_pos = newline_pos - 1;
	switch (line[multiplier_pos]) {
		case 'K':
			multiplier = 1024;
		break;
		case 'M':
			multiplier = 1024 * 1024;
		break;
		case 'G':
			multiplier = 1024 * 1024 * 1024;
		break;
	}

	// Remove multiplier
	if (multiplier != 1) {
		line[multiplier_pos] = '\0';
	}

	// Line should now be clear of non-numeric characters
	int value = 0;
	if (arg == 0) {
		value = atoi(line);
	}
	else {
		value = atoi(line)/11 * arg;
	}

	int cache_size = value * multiplier;

	fclose(cache_size_fd);

	return cache_size;
}

int main(int argc, char ** argv)
{
    int i = 0;
    int ret, fd;
    unsigned int step = 0;
    unsigned long int map_size;
    int use_mmap_driver = 0;
    int *address = NULL;
    int *orig_addr = NULL;
    unsigned long int offset;
    struct timespec start_time;
    struct timespec end_time;

    if (argc != NUM_EXPECTED_ARGS)
    {
      printf("Incorrect number of arguments\n");
      usage();
      exit(EXIT_FAILURE);
    }

    sscanf(argv[1], "%lu", &map_size);
    sscanf(argv[2], "%lu", &offset);
    use_mmap_driver = atoi(argv[3]);
    
    if (map_size % 2 != 0)
    {
        printf("Size must be even\n");
        usage();
        exit(EXIT_FAILURE);
    }
    
    if (offset < 0)
    {
        printf("Offset must be greater than 0\n");
        usage();
        exit(EXIT_FAILURE);
    }

    get_time(&start_time);
    
q    fd = open(DEV_NAME, O_RDWR);
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
    orig_addr = address;
    
    
    if (address == MAP_FAILED)
    {
        exit(EXIT_FAILURE);
    }    

    while((offset * step) < map_size) 
    {
		*address = step;
		address += offset/sizeof(int);
		step++;
	}
    
    munmap(orig_addr, map_size);
    get_time(&end_time);
    printf("Execution time: %.9f secs\n", timediff(start_time, end_time));
    
    return ret;   
}
