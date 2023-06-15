#ifndef __SPI_REG_STRUCT_H__
#define __SPI_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct spi_reg {

	union {
		struct {
			/**
			bitpos: [[3:0]]
			data frame size
			 */
			uint32_t dfs:			4;
			/**
			bitpos: [[5:4]]
			00:Motorola SPI
			01:TI SSP
			10:National semiconductors MicroWire
			11:reserved
			 */
			uint32_t frf:			2;
			/**
			bitpos: [[6]]
			0: Inactive state of serial clock is low
			1: Inactive state of serial clock is high
			 */
			uint32_t scph:			1;
			/**
			bitpos: [[7]]
			0: Inactive state of serial clock is low
			1: Inactive state of serial clock is high
			 */
			uint32_t scpol:			1;
			/**
			bitpos: [[9:8]]
			00: TX & RX
			01: TX only
			10: RX only
			11: eeprom read
			 */
			uint32_t tmod:			2;
			/**
			bitpos: [[10]]
			0: no toggle
			1: toggle cs between data frames
			 */
			uint32_t cs_tgl:			1;
			/**
			bitpos: [[11]]
			0: normal mode
			1: test mode
			 */
			uint32_t srl_test:			1;
			/**
			bitpos: [[15:12]]
			control frame size for MicroWire frame format
			 */
			uint32_t cfs:			4;
			uint32_t reserved16:			16;
		};
		uint32_t val;
	} ctrl0;	/* REG_SPI_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[15:0]]
			number of data frames, actually plus 1
			 */
			uint32_t ndf:			16;
			uint32_t reserved16:			16;
		};
		uint32_t val;
	} ctrl1;	/* REG_SPI_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t ssi_en:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} ssienr;	/* REG_SPI_BASE + 0x8 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Microwire handshaking
			 */
			uint32_t mwmod:			1;
			/**
			bitpos: [[1]]
			Microwire control
			 */
			uint32_t mdd:			1;
			/**
			bitpos: [[2]]
			Microwire transfer mode
			 */
			uint32_t mhs:			1;
			uint32_t reserved3:			29;
		};
		uint32_t val;
	} mvcr;	/* REG_SPI_BASE + 0xc */

	union {
		struct {
			/**
			bitpos: [[0]]
			 */
			uint32_t ser:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} ser;	/* REG_SPI_BASE + 0x10 */

	union {
		struct {
			/**
			bitpos: [[15:0]]
			ssi clock divder
			 */
			uint32_t sckdv:			16;
			uint32_t reserved16:			16;
		};
		uint32_t val;
	} baudr;	/* REG_SPI_BASE + 0x14 */

	union {
		struct {
			/**
			bitpos: [[3:0]]
			tx fifo threshold, <= value will assert ssi_txe_intr(txeir)
			 */
			uint32_t tft:			4;
			uint32_t reserved4:			28;
		};
		uint32_t val;
	} txftlr;	/* REG_SPI_BASE + 0x18 */

	union {
		struct {
			/**
			bitpos: [[3:0]]
			rx fifo threshold, >= (value+1) will assert ssi_rxf_intr(rxfir)
			 */
			uint32_t rft:			4;
			uint32_t reserved4:			28;
		};
		uint32_t val;
	} rxftlr;	/* REG_SPI_BASE + 0x1c */

	union {
		struct {
			/**
			bitpos: [[4:0]]
			tx fifo level
			 */
			uint32_t txtfl:			5;
			uint32_t reserved5:			27;
		};
		uint32_t val;
	} txflr;	/* REG_SPI_BASE + 0x20 */

	union {
		struct {
			/**
			bitpos: [[4:0]]
			rx fifo level
			 */
			uint32_t rxtfl:			5;
			uint32_t reserved5:			27;
		};
		uint32_t val;
	} rxflr;	/* REG_SPI_BASE + 0x24 */

	union {
		struct {
			/**
			bitpos: [[0]]
			ssi busy flag
			 */
			uint32_t busy:			1;
			/**
			bitpos: [[1]]
			tx fifo not full
			 */
			uint32_t tfnf:			1;
			/**
			bitpos: [[2]]
			tx fifo empty
			 */
			uint32_t tfe:			1;
			/**
			bitpos: [[3]]
			rx fifo not empty
			 */
			uint32_t rfne:			1;
			/**
			bitpos: [[4]]
			rx fifo full
			 */
			uint32_t rff:			1;
			uint32_t reserved5:			1;
			/**
			bitpos: [[6]]
			tx data collision error
			 */
			uint32_t dcol:			1;
			uint32_t reserved7:			25;
		};
		uint32_t val;
	} sr;	/* REG_SPI_BASE + 0x28 */

	union {
		struct {
			/**
			bitpos: [[0]]
			tx fifo empty interrupt mask
			 */
			uint32_t txeim:			1;
			/**
			bitpos: [[1]]
			tx fifo overflow interrupt mask
			 */
			uint32_t txoim:			1;
			/**
			bitpos: [[2]]
			rx fifo underflow interrupt mask
			 */
			uint32_t rxuim:			1;
			/**
			bitpos: [[3]]
			rx fifo overflow interrupt mask
			 */
			uint32_t rxoim:			1;
			/**
			bitpos: [[4]]
			rx fifo full interrupt mask
			 */
			uint32_t rxfim:			1;
			/**
			bitpos: [[5]]
			spi master done interrupt mask
			 */
			uint32_t mstim:			1;
			uint32_t reserved6:			26;
		};
		uint32_t val;
	} imr;	/* REG_SPI_BASE + 0x2c */

	union {
		struct {
			/**
			bitpos: [[0]]
			tx fifo empty interrupt status
			 */
			uint32_t txeis:			1;
			/**
			bitpos: [[1]]
			tx fifo overflow interrupt status
			 */
			uint32_t txois:			1;
			/**
			bitpos: [[2]]
			rx fifo underflow interrupt status
			 */
			uint32_t rxuis:			1;
			/**
			bitpos: [[3]]
			rx fifo overflow interrupt status
			 */
			uint32_t rxois:			1;
			/**
			bitpos: [[4]]
			rx fifo full interrupt status
			 */
			uint32_t rxfis:			1;
			/**
			bitpos: [[5]]
			spi master done interrupt status
			 */
			uint32_t mstis:			1;
			uint32_t reserved6:			26;
		};
		uint32_t val;
	} isr;	/* REG_SPI_BASE + 0x30 */

	union {
		struct {
			/**
			bitpos: [[0]]
			tx fifo empty raw interrupt status
			 */
			uint32_t txeir:			1;
			/**
			bitpos: [[1]]
			tx fifo overflow raw interrupt status
			 */
			uint32_t txoir:			1;
			/**
			bitpos: [[2]]
			rx fifo underflow raw interrupt status
			 */
			uint32_t rxuir:			1;
			/**
			bitpos: [[3]]
			rx fifo overflow raw interrupt status
			 */
			uint32_t rxoir:			1;
			/**
			bitpos: [[4]]
			rx fifo full raw interrupt status
			 */
			uint32_t rxfir:			1;
			/**
			bitpos: [[5]]
			spi master done raw interrupt status
			 */
			uint32_t mstir:			1;
			uint32_t reserved6:			26;
		};
		uint32_t val;
	} risr;	/* REG_SPI_BASE + 0x34 */

	union {
		struct {
			/**
			bitpos: [[0]]
			clear tx fifo overflow int
			 */
			uint32_t txoicr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} txoicr;	/* REG_SPI_BASE + 0x38 */

	union {
		struct {
			/**
			bitpos: [[0]]
			clear rx fifo overflow int
			 */
			uint32_t rxoicr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} rxoicr;	/* REG_SPI_BASE + 0x3c */

	union {
		struct {
			/**
			bitpos: [[0]]
			clear rx fifo underflow int
			 */
			uint32_t rxuicr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} rxuicr;	/* REG_SPI_BASE + 0x40 */

	union {
		struct {
			/**
			bitpos: [[0]]
			clear spi master done int
			 */
			uint32_t msticr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} msticr;	/* REG_SPI_BASE + 0x44 */

	union {
		struct {
			/**
			bitpos: [[0]]
			clear int
			 */
			uint32_t icr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} icr;	/* REG_SPI_BASE + 0x48 */

	union {
		struct {
			/**
			bitpos: [[0]]
			DMA start trigger
			 */
			uint32_t dmacr:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} dmacr;	/* REG_SPI_BASE + 0x4c */

	union {
		struct {
			/**
			bitpos: [[15:0]]
			DMA spi command
			 */
			uint32_t slv_cmd:			16;
			/**
			bitpos: [[16]]
			DMA spi slave command length(per dfs)
			0: no slave command phase
			1: 1 dfs
			 */
			uint32_t slv_cl:			1;
			uint32_t reserved17:			15;
		};
		uint32_t val;
	} dmacmdr;	/* REG_SPI_BASE + 0x50 */

	union {
		struct {
			/**
			bitpos: [[23:0]]
			DMA spi slave address
			 */
			uint32_t slv_a:			24;
			/**
			bitpos: [[25:24]]
			DMA spi slave address length(per dfs)
			00: no slave address phase
			01: 1 dfs
			10: 2 dfs
			11: 3 dfs
			 */
			uint32_t slv_al:			2;
			uint32_t reserved26:			6;
		};
		uint32_t val;
	} dmasar;	/* REG_SPI_BASE + 0x54 */
	/**
 DMA IMB write address
	 */
	uint32_t dmamwar; /* REG_SPI_BASE + 0x58 */
	/**
 DMA IMB read address
	 */
	uint32_t dmamrar; /* REG_SPI_BASE + 0x5c */

	union {
		struct {
			/**
			bitpos: [[7:0]]
			DMA IMB latency
			 */
			uint32_t dmalatr:			8;
			uint32_t reserved8:			24;
		};
		uint32_t val;
	} dmalatr;	/* REG_SPI_BASE + 0x60 */

	union {
		struct {
			/**
			bitpos: [[0]]
			1: Refer to ILI9341.pdf. Spi has 3 wire: cs, sclk, sda.
			   Sda is a bi-didrectional io for txd, rxd.
			0: normal mode
			 */
			uint32_t spi_lcd_ifl:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} spi_lcd_ifl;	/* REG_SPI_BASE + 0x64 */

	uint32_t reserved_68; /* REG_SPI_BASE + 0x68 */
	uint32_t reserved_6c; /* REG_SPI_BASE + 0x6c */
	/**
 read  = rx fifo buffer
 write = tx fifo buffer
	 */
	uint32_t dr_low[32]; /* REG_SPI_BASE + 0x70 */

#if 0
	uint32_t reserved_74; /* REG_SPI_BASE + 0x74 */
	uint32_t reserved_78; /* REG_SPI_BASE + 0x78 */
	uint32_t reserved_7c; /* REG_SPI_BASE + 0x7c */
	uint32_t reserved_80; /* REG_SPI_BASE + 0x80 */
	uint32_t reserved_84; /* REG_SPI_BASE + 0x84 */
	uint32_t reserved_88; /* REG_SPI_BASE + 0x88 */
	uint32_t reserved_8c; /* REG_SPI_BASE + 0x8c */
	uint32_t reserved_90; /* REG_SPI_BASE + 0x90 */
	uint32_t reserved_94; /* REG_SPI_BASE + 0x94 */
	uint32_t reserved_98; /* REG_SPI_BASE + 0x98 */
	uint32_t reserved_9c; /* REG_SPI_BASE + 0x9c */
	uint32_t reserved_a0; /* REG_SPI_BASE + 0xa0 */
	uint32_t reserved_a4; /* REG_SPI_BASE + 0xa4 */
	uint32_t reserved_a8; /* REG_SPI_BASE + 0xa8 */
	uint32_t reserved_ac; /* REG_SPI_BASE + 0xac */
	uint32_t reserved_b0; /* REG_SPI_BASE + 0xb0 */
	uint32_t reserved_b4; /* REG_SPI_BASE + 0xb4 */
	uint32_t reserved_b8; /* REG_SPI_BASE + 0xb8 */
	uint32_t reserved_bc; /* REG_SPI_BASE + 0xbc */
	uint32_t reserved_c0; /* REG_SPI_BASE + 0xc0 */
	uint32_t reserved_c4; /* REG_SPI_BASE + 0xc4 */
	uint32_t reserved_c8; /* REG_SPI_BASE + 0xc8 */
	uint32_t reserved_cc; /* REG_SPI_BASE + 0xcc */
	uint32_t reserved_d0; /* REG_SPI_BASE + 0xd0 */
	uint32_t reserved_d4; /* REG_SPI_BASE + 0xd4 */
	uint32_t reserved_d8; /* REG_SPI_BASE + 0xd8 */
	uint32_t reserved_dc; /* REG_SPI_BASE + 0xdc */
	uint32_t reserved_e0; /* REG_SPI_BASE + 0xe0 */
	uint32_t reserved_e4; /* REG_SPI_BASE + 0xe4 */
	uint32_t reserved_e8; /* REG_SPI_BASE + 0xe8 */
	uint32_t reserved_ec; /* REG_SPI_BASE + 0xec */
#endif

	union {
		struct {
			/**
			bitpos: [[7:0]]
			rxd sample delay
			 */
			uint32_t rsd:			8;
			uint32_t reserved8:			24;
		};
		uint32_t val;
	} rx_smp_dly;	/* REG_SPI_BASE + 0xf0 */

	union {
		struct {
			/**
			bitpos: [[7:0]]
			0x0: no dummy cycle
			0x1: 1 dummy cycle
			0x2: 2 dummy cycle
			…
			0xff: 127 dummy cycle
			 */
			uint32_t dmy_cyc:			8;
			uint32_t reserved8:			24;
		};
		uint32_t val;
	} spi_ctrlr0;	/* REG_SPI_BASE + 0xf4 */

	union {
		struct {
			/**
			bitpos: [[0]]
			0x0: unididrectional sda
			0x1: bi-didrectional sda
			 */
			uint32_t spi_bi_sda_0:			1;
			/**
			bitpos: [[1]]
			0x1: address&data phase dual io
			0x0: data phase dual io
			 */
			uint32_t spi_bi_sda_1:			1;
			uint32_t reserved2:			30;
		};
		uint32_t val;
	} spi_bi_sda;	/* REG_SPI_BASE + 0xf8 */

	union {
		struct {
			/**
			bitpos: [[0]]
			0x0: MSB first
			0x1: LSB first
			 */
			uint32_t spi_lsbf:			1;
			uint32_t reserved1:			31;
		};
		uint32_t val;
	} spi_lsbf;	/* REG_SPI_BASE + 0xfc */

} spi_reg_t;

extern spi_reg_t SPI0;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __SPI_REG_STRUCT_H__ */
