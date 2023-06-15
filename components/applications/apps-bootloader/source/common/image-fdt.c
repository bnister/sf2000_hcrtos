#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <image.h>
#include <kernel/lib/libfdt/libfdt.h>

static const image_header_t *image_get_fdt(ulong fdt_addr)
{
	const image_header_t *fdt_hdr = (const image_header_t *)fdt_addr;

	image_print_contents(fdt_hdr);

	puts("   Verifying Checksum ... ");
	if (!image_check_hcrc(fdt_hdr)) {
		printf("fdt header checksum invalid");
		return NULL;
	}

	if (!image_check_dcrc(fdt_hdr)) {
		printf("fdt checksum invalid");
		return NULL;
	}
	puts("OK\n");

	if (!image_check_type(fdt_hdr, IH_TYPE_FLATDT)) {
		printf("uImage is not a fdt");
		return NULL;
	}
	if (image_get_comp(fdt_hdr) != IH_COMP_NONE) {
		printf("uImage is compressed");
		return NULL;
	}
	if (fdt_check_header((void *)image_get_data(fdt_hdr)) != 0) {
		printf("uImage data is not a fdt");
		return NULL;
	}
	return fdt_hdr;
}

/**
 * boot_get_fdt - main fdt handling routine
 * @argc: command argument count
 * @argv: command argument list
 * @arch: architecture (IH_ARCH_...)
 * @images: pointer to the bootm images structure
 * @of_flat_tree: pointer to a char* variable, will hold fdt start address
 * @of_size: pointer to a unsigned long variable, will hold fdt length
 *
 * boot_get_fdt() is responsible for finding a valid flat device tree image.
 * Curently supported are the following ramdisk sources:
 *      - multicomponent kernel/ramdisk image,
 *      - commandline provided address of decicated ramdisk image.
 *
 * returns:
 *     0, if fdt image was found and valid, or skipped
 *     of_flat_tree and of_size are set to fdt start address and length if
 *     fdt image is found and valid
 *
 *     1, if fdt image is found but corrupted
 *     of_flat_tree and of_size are set to 0 if no fdt exists
 */
int boot_get_fdt(int flag, int argc, char *const argv[], uint8_t arch,
		 bootm_headers_t *images, char **of_flat_tree, unsigned long *of_size)
{
	const image_header_t *fdt_hdr;
	unsigned long		load, load_end;
	unsigned long		image_start, image_data, image_end;
	unsigned long		img_addr;
	unsigned long		fdt_addr;
	char		*fdt_blob = NULL;
	void		*buf;
	const char *select = NULL;

	*of_flat_tree = NULL;
	*of_size = 0;

	img_addr = (argc == 0) ? 0 : strtoul(argv[0], NULL, 16);
	buf = (void *)img_addr;

	if (argc > 2)
		select = argv[2];

	if (select || genimg_has_config(images)) {
		{
			fdt_addr = strtoul(select, NULL, 16);
			printf("*  fdt: cmdline image address = 0x%08lx\n",
			      fdt_addr);
		}
		printf("## Checking for 'FDT'/'FDT Image' at %08lx\n",
		      fdt_addr);

		/*
		 * Check if there is an FDT image at the
		 * address provided in the second bootm argument
		 * check image type, for FIT images get a FIT node.
		 */
		buf = (void *)fdt_addr;
		switch (genimg_get_format(buf)) {
		case IMAGE_FORMAT_LEGACY:
			/* verify fdt_addr points to a valid image header */
			printf("## Flattened Device Tree from Legacy Image at %08lx\n",
			       fdt_addr);
			fdt_hdr = image_get_fdt(fdt_addr);
			if (!fdt_hdr)
				goto no_fdt;

			/*
			 * move image data to the load address,
			 * make sure we don't overwrite initial image
			 */
			image_start = (unsigned long)fdt_hdr;
			image_data = (unsigned long)image_get_data(fdt_hdr);
			image_end = image_get_image_end(fdt_hdr);

			load = image_get_load(fdt_hdr);
			load_end = load + image_get_data_size(fdt_hdr);

			if (load == image_start ||
			    load == image_data) {
				fdt_addr = load;
				break;
			}

			if ((load < image_end) && (load_end > image_start)) {
				printf("fdt overwritten");
				goto error;
			}

			printf("   Loading FDT from 0x%08lx to 0x%08lx\n",
			      image_data, load);

			memmove((void *)load,
				(void *)image_data,
				image_get_data_size(fdt_hdr));

			fdt_addr = load;
			break;
		case IMAGE_FORMAT_FIT:
			/*
			 * This case will catch both: new uImage format
			 * (libfdt based) and raw FDT blob (also libfdt
			 * based).
			 */
			{
				/*
				 * FDT blob
				 */
				printf("*  fdt: raw FDT blob\n");
				printf("## Flattened Device Tree blob at %08lx\n",
				       (long)fdt_addr);
			}
			break;
		default:
			puts("ERROR: Did not find a cmdline Flattened Device Tree\n");
			goto error;
		}

		printf("   Booting using the fdt blob at %#08lx\n", fdt_addr);
		fdt_blob = (char *)fdt_addr;
	} else {
		printf("## No Flattened Device Tree\n");
		goto no_fdt;
	}

	*of_flat_tree = fdt_blob;
	*of_size = fdt_totalsize(fdt_blob);
	printf("   of_flat_tree at 0x%08lx size 0x%08lx\n",
	      (unsigned long)*of_flat_tree, *of_size);

	return 0;

no_fdt:
	printf("Continuing to boot without FDT\n");
	return 0;
error:
	return 1;
}
