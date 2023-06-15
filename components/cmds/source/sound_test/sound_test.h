#ifndef __SOUND_TEST_H
#define __SOUND_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/poll.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <libavutil/common.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/kshm.h>

#include <ffplayer.h>
#include <glist.h>
#include "alac_dec.h"

int i2so_test(int argc, char *argv[]);
int i2si_test(int argc, char *argv[]);
int i2s_rec_play_test(int argc, char *argv[]);
int pcm_rec_play_test(int argc, char *argv[]);
int wm8960_test(int argc, char *argv[]);
int stop_i2si_test(int argc, char *argv[]);
int stop_i2s_rec_play_test(int argc, char *argv[]);
int stop_pcm_rec_play_test(int argc, char *argv[]);
int tdmi_test(int argc, char *argv[]);
int spin_test(int argc, char *argv[]);
int pdmi_test(int argc, char *argv[]);
int pdmi_test(int argc, char *argv[]);
int i2si012_test(int argc, char *argv[]);
int stop_i2si012_test(int argc, char *argv[]);
int pcmi_test(int argc, char *argv[]);
int stop_pcmi_test(int argc, char *argv[]);
int i2si_i2so_recoder_test(int argc, char *argv[]);
int i2si_spectrum_test(int argc, char *argv[]);
int i2so_spectrum_test(int argc, char *argv[]);
int stop_i2si_spectrum_test(int argc, char *argv[]);
int stop_i2so_spectrum_test(int argc, char *argv[]);
int aec_test(int argc, char *argv[]);
int stop_aec_test(int argc, char *argv[]);
int aec_main(int argc, char *argv[]);
int dsp_test (int argc, char *argv[]);
int stop_dsp_test(int argc, char *argv[]);

#endif /* __SOUND_TEST_H */
