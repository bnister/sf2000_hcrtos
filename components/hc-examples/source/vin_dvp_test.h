#ifndef __VIN_DVP_TEST__
#define __VIN_DVP_TEST__

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

int vindvp_enter(int argc , char *argv[]);
int vindvp_stop(int argc , char *argv[]);
int vindvp_start(int argc , char *argv[]);
int vindvp_enable(int argc , char *argv[]);
int vindvp_disable(int argc , char *argv[]);
int vindvp_set_combine_regin_status(int argc , char *argv[]);
int vindvp_capture_pictrue(int argc , char *argv[]);

#endif
