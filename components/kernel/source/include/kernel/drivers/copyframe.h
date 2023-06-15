#ifndef _AVDRIVER_AUDSINK_COPYFRAME_H_
#define _AVDRIVER_AUDSINK_COPYFRAME_H_

#include <stdint.h>
#include <hcuapi/snd.h>

void audsink_copy_raw(void *dst, void *src, snd_pcm_uframes_t frames);
void audsink_copy_s8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup);
void audsink_copy_u8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup);
void audsink_copy_s16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup);
void audsink_copy_s16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup);
void audsink_copy_u16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup);
void audsink_copy_u16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup);
void audsink_copy_s24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align);
void audsink_copy_u24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align);
void audsink_copy_s32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup);
void audsink_copy_s32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup);
void audsink_copy_u32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup);
void audsink_copy_u32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup);

#endif
