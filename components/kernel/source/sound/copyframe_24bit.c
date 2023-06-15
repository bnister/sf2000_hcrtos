#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <nuttx/fs/fs.h>
#include <kernel/io.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <hcuapi/audsink.h>
#include <generated/br2_autoconf.h>

#define _READ(__data, __idx, __size, __shift) \
    (((__uint##__size##_t) (((const unsigned char *) (__data))[__idx])) << (__shift))

#define S16BETOS24R(pin) (_READ(pin, 0, 32, 16) | _READ(pin, 1, 32, 8))
#define U16BETOS24R(pin) (S16BETOS24R(pin) - 0x800000)
#define S32BETOS24R(pin) (_READ(pin, 0, 32, 16) | _READ(pin, 1, 32, 8) | _READ(pin, 2, 32, 0))
#define U32BETOS24R(pin) (S32BETOS24R(pin) - 0x800000)

#define S8TOS24R(in) ((int32_t)(in) << 16)
#define U8TOS24R(in) ((int32_t)((in) - 0x80) << 16)
#define S16TOS24R(in) ((int32_t)(in) << 8)
#define U16TOS24R(in) ((int32_t)((in) - 0x8000) << 8)
#define S24LTOS24R(in) ((int32_t)(in) >> 8)
#define S24RTOS24R(in) ((int32_t)(in))
#define U24LTOS24R(in) ((int32_t)((in) - 0x80000000) >> 8)
#define U24RTOS24R(in) ((int32_t)((in) - 0x800000))
#define S32TOS24R(in) ((int32_t)(in) >> 8)
#define U32TOS24R(in) ((int32_t)((in) - 0x80000000) >> 8)

void audsink_copy_raw(void *dst, void *src, snd_pcm_uframes_t frames)
{
	memcpy(dst, src, frames);
}

void audsink_copy_s8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup)
{
	char *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (char *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (char *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (char *)src;
					in2 = (char *)src + 1;
					break;
				}
			} else {
				in = (char *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (char *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (char *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (char *)src;
					in2 = (char *)src + pitch;
					break;
				}
			} else {
				in = (char *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S8TOS24R(*in) + S8TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S8TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (char *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S8TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u8(void *dst, void *src, snd_pcm_uframes_t frames,
		     snd_pcm_uframes_t pitch, uint8_t channels_in,
		     snd_pcm_access_t access, int dup)
{
	unsigned char *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (unsigned char *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (unsigned char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (unsigned char *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (unsigned char *)src;
					in2 = (unsigned char *)src + 1;
					break;
				}
			} else {
				in = (unsigned char *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (unsigned char *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (unsigned char *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (unsigned char *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (unsigned char *)src;
					in2 = (unsigned char *)src + pitch;
					break;
				}
			} else {
				in = (unsigned char *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U8TOS24R(*in) + U8TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U8TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (unsigned char *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U8TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	int16_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + 1;
					break;
				}
			} else {
				in = (int16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (int16_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S16TOS24R(*in) + S16TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S16TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int16_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S16TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	int16_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + 1;
					break;
				}
			} else {
				in = (int16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (int16_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S16BETOS24R(in) + S16BETOS24R(in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S16BETOS24R(in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int16_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S16BETOS24R(in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u16(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	uint16_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (uint16_t *)src + 1;
					break;
				}
			} else {
				in = (uint16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (uint16_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U16TOS24R(*in) + U16TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U16TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint16_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U16TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u16_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	uint16_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (uint16_t *)src + 1;
					break;
				}
			} else {
				in = (uint16_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint16_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint16_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint16_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint16_t *)src;
					in2 = (int16_t *)src + pitch;
					break;
				}
			} else {
				in = (uint16_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U16BETOS24R(in) + U16BETOS24R(in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U16BETOS24R(in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint16_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U16BETOS24R(in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align)
{
	int32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + 1;
					break;
				}
			} else {
				in = (int32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + pitch;
					break;
				}
			} else {
				in = (int32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = (S24LTOS24R(*in) + S24LTOS24R(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = (S24RTOS24R(*in) + S24RTOS24R(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			}
			break;
		} else {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = S24LTOS24R(*in);
					out += ostep;
					in += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = S24RTOS24R(*in);
					out += ostep;
					in += istep;
				}
			}
		}
	}

	if (channels_in == 1) {
		in = (int32_t *)src;
		out = (int32_t *)dst + 1;

		if (align == SND_PCM_ALIGN_LEFT) {
			for (i = 0; i < frames; i++) {
				*out = S24LTOS24R(*in);
				out += ostep;
				in++;
			}
		} else {
			for (i = 0; i < frames; i++) {
				*out = S24RTOS24R(*in);
				out += ostep;
				in++;
			}
		}
	}

	return;
}

void audsink_copy_u24(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup, uint8_t align)
{
	uint32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = (U24LTOS24R(*in) + U24LTOS24R(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = (U24RTOS24R(*in) + U24RTOS24R(*in2)) >> 1;
					*(out + 1) = *out;
					out += ostep;
					in += istep;
					in2 += istep;
				}
			}
			break;
		} else {
			if (align == SND_PCM_ALIGN_LEFT) {
				for (i = 0; i < frames; i++) {
					*out = U24LTOS24R(*in);
					out += ostep;
					in += istep;
				}
			} else {
				for (i = 0; i < frames; i++) {
					*out = U24RTOS24R(*in);
					out += ostep;
					in += istep;
				}
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int32_t *)dst + 1;

		if (align == SND_PCM_ALIGN_LEFT) {
			for (i = 0; i < frames; i++) {
				*out = U24LTOS24R(*in);
				out += ostep;
				in++;
			}
		} else {
			for (i = 0; i < frames; i++) {
				*out = U24RTOS24R(*in);
				out += ostep;
				in++;
			}
		}
	}

	return;
}

void audsink_copy_s32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	int32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + 1;
					break;
				}
			} else {
				in = (int32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + pitch;
					break;
				}
			} else {
				in = (int32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S32TOS24R(*in) + S32TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S32TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int32_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S32TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_s32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	int32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + 1;
					break;
				}
			} else {
				in = (int32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (int32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (int32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (int32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (int32_t *)src;
					in2 = (int32_t *)src + pitch;
					break;
				}
			} else {
				in = (int32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (S32BETOS24R(*in) + S32BETOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = S32BETOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (int32_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = S32BETOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u32(void *dst, void *src, snd_pcm_uframes_t frames,
		      snd_pcm_uframes_t pitch, uint8_t channels_in,
		      snd_pcm_access_t access, int dup)
{
	uint32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U32TOS24R(*in) + U32TOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U32TOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U32TOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}

void audsink_copy_u32_be(void *dst, void *src, snd_pcm_uframes_t frames,
			 snd_pcm_uframes_t pitch, uint8_t channels_in,
			 snd_pcm_access_t access, int dup)
{
	uint32_t *in, *in2;
	int32_t *out;
	uint8_t ch;
	int istep;
	int ostep;
	snd_pcm_uframes_t i;

	if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
		istep = channels_in;
	} else {
		istep = 1;
	}

	if (channels_in == 1) {
		ostep = 2;
	} else {
		ostep = channels_in;
	}

	for (ch = 0; ch < channels_in; ch++) {
		in2 = NULL;
		if (access == SND_PCM_ACCESS_RW_INTERLEAVED) {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + 1;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + 1;
					break;
				}
			} else {
				in = (uint32_t *)src + ch;
			}
		} else {
			if (channels_in == 2) {
				switch (dup) {
				case AUDSINK_PCM_DUPLICATE_STEREO:
					in = (uint32_t *)src + ch * pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_LEFT:
					in = (uint32_t *)src;
					break;
				case AUDSINK_PCM_DUPLICATE_RIGHT:
					in = (uint32_t *)src + pitch;
					break;
				case AUDSINK_PCM_DUPLICATE_MONO:
				default:
					in = (uint32_t *)src;
					in2 = (uint32_t *)src + pitch;
					break;
				}
			} else {
				in = (uint32_t *)src + ch * pitch;
			}
		}

		out = (int32_t *)dst + ch;

		if (in2) {
			for (i = 0; i < frames; i++) {
				*out = (U32BETOS24R(*in) + U32BETOS24R(*in2)) >> 1;
				*(out + 1) = *out;
				out += ostep;
				in += istep;
				in2 += istep;
			}
			break;
		} else {
			for (i = 0; i < frames; i++) {
				*out = U32BETOS24R(*in);
				out += ostep;
				in += istep;
			}
		}
	}

	if (channels_in == 1) {
		in = (uint32_t *)src;
		out = (int32_t *)dst + 1;

		for (i = 0; i < frames; i++) {
			*out = U32BETOS24R(*in);
			out += ostep;
			in++;
		}
	}

	return;
}
