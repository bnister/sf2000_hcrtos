#include "sound_test.h"

static int sound_test(int argc, char *argv[]){
	return 0;
}

CONSOLE_CMD(snd, NULL, sound_test, CONSOLE_CMD_MODE_SELF,
    "enter sound test")
CONSOLE_CMD(i2so, "snd", i2so_test, CONSOLE_CMD_MODE_SELF,
    "do i2so test")
CONSOLE_CMD(i2si, "snd", i2si_test, CONSOLE_CMD_MODE_SELF,
    "do i2si test")
CONSOLE_CMD(i2s_rec_play, "snd", i2s_rec_play_test, CONSOLE_CMD_MODE_SELF,
	"do i2s rec play test")
CONSOLE_CMD(pcm_rec_play, "snd", pcm_rec_play_test, CONSOLE_CMD_MODE_SELF,
	"do pcm rec play test")
CONSOLE_CMD(wm8960, "snd", wm8960_test, CONSOLE_CMD_MODE_SELF,
    "before doing i2si test,need to do wm8960_test")
CONSOLE_CMD(i2si012, "snd", i2si012_test, CONSOLE_CMD_MODE_SELF,
	"do i2si0/i2si1/i2si2 test")
CONSOLE_CMD(stop_i2si012, "snd", stop_i2si012_test, CONSOLE_CMD_MODE_SELF,
	"stop i2si test")
CONSOLE_CMD(pcmi, "snd", pcmi_test, CONSOLE_CMD_MODE_SELF,
	"do pcmi0/pcmi1/pcmi2 test")
CONSOLE_CMD(stop_pcmi, "snd", stop_pcmi_test, CONSOLE_CMD_MODE_SELF,
	"stop pcmi0/pcmi1/pcmi2 test")
CONSOLE_CMD(stop_i2si, "snd", stop_i2si_test, CONSOLE_CMD_MODE_SELF,
	"stop i2si test")
CONSOLE_CMD(stop_i2s_rec_play, "snd", stop_i2s_rec_play_test, CONSOLE_CMD_MODE_SELF,
	"stop i2s rec play test")
CONSOLE_CMD(stop_pcm_rec_play, "snd", stop_pcm_rec_play_test, CONSOLE_CMD_MODE_SELF,
	"stop pcm rec play test")
CONSOLE_CMD(tdmi, "snd", tdmi_test, CONSOLE_CMD_MODE_SELF,
	"do tdmi test")
CONSOLE_CMD(spin, "snd", spin_test, CONSOLE_CMD_MODE_SELF,
	"do spin test")
CONSOLE_CMD(pdmi, "snd", pdmi_test, CONSOLE_CMD_MODE_SELF,
	"do pdmi test")
CONSOLE_CMD(i2si_i2so, "snd", i2si_i2so_recoder_test, CONSOLE_CMD_MODE_SELF,
	"do i2si_i2so test")
CONSOLE_CMD(i2so_spectrum, "snd", i2so_spectrum_test, CONSOLE_CMD_MODE_SELF,
	"do i2so_spectrum test")
CONSOLE_CMD(i2si_spectrum, "snd", i2si_spectrum_test, CONSOLE_CMD_MODE_SELF,
	"do i2si_spectrum test")
CONSOLE_CMD(stop_spectrum_i2si, "snd", stop_i2si_spectrum_test, CONSOLE_CMD_MODE_SELF,
	"stop i2si spectrum test")
CONSOLE_CMD(stop_spectrum_i2so, "snd", stop_i2so_spectrum_test, CONSOLE_CMD_MODE_SELF,
	"stop i2so spectrum test")
CONSOLE_CMD(aec_rec, "snd", aec_test, CONSOLE_CMD_MODE_SELF,"do aec rec")
CONSOLE_CMD(aec_stop, "snd", stop_aec_test, CONSOLE_CMD_MODE_SELF,"stop aec test")
CONSOLE_CMD(aec_start, "snd", aec_main, CONSOLE_CMD_MODE_SELF,"do aec test")
CONSOLE_CMD(alac, "snd", alac_test, CONSOLE_CMD_MODE_SELF,"do alac test")
CONSOLE_CMD(dsp, "snd", dsp_test, CONSOLE_CMD_MODE_SELF,"do dsp test")
CONSOLE_CMD(dsp_stop, "snd", stop_dsp_test, CONSOLE_CMD_MODE_SELF,"stop dsp test")

