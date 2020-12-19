#ifndef __PAGING_H
#define __PAGING_H

/* Seen by kernel and user */
#define PAGING_MODULE_NAME "paging"
#define DEV_NAME "/dev/" PAGING_MODULE_NAME

#endif

double timediff(struct timespec ti, struct timespec tf)
{
    return (double)((tf.tv_sec - ti.tv_sec)
      + (tf.tv_nsec - ti.tv_nsec)
      / 1E9);
}

void get_time(struct timespec *time)
{
    clock_gettime(CLOCK_MONOTONIC_RAW, time);
}

int get_rand(int min, int max)
{ 
    return (rand() % (max - min + 1)) + min; 
}
