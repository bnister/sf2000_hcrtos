#ifndef __UART_REG_STRUCT_H__
#define __UART_REG_STRUCT_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#endif

typedef volatile struct uart_reg {

	union {
		struct {
			/**
			bitpos: [[7:0]]
			UART Receiver Buffer Register (URBR)
			This register receives and holds the entering data.

			UART Transmitter Buffer Register (UTBR)
			This register holds and transmits the data via a non-accessible shift register.
			 */
			uint8_t rev_trans_buffer:			8;
		};
		uint8_t val;
	} urbr_utbr;	/* REG_UART_BASE + 0x0 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Enable Received Data Available Interrupt (ERDVI)
			Sets this bit high to enable the Received Data Available Interrupt (and Time-out Interrupt in the FIFO mode)
			 */
			uint8_t erdvi:			1;
			/**
			bitpos: [[1]]
			Enable Transmitter Holding Register Empty Interrupt(ETHREI)
			Sets this bit high to enable the Transmitter Holding Register Empty Interrupt .
			 */
			uint8_t ethrei:			1;
			/**
			bitpos: [[2]]
			Enable Receiver Line Status Interrupt(ERLSI)
			Sets this bit high to enable the Receiver Line Status Interrupt ,which is caused when Overrun,Parity,Framing or Break occurs.
			 */
			uint8_t erlsi:			1;
			/**
			bitpos: [[3]]
			Enable Modem Status Interrupt(EMSI)
			Sets this bit high to enable the Modem Status Interrupt when one of the Modem Status Registers changes its bit state
			 */
			uint8_t emsi:			1;
			uint8_t reserved4:			4;
		};
		uint8_t val;
	} uier;	/* REG_UART_BASE + 0x1 */

	union {
		struct {
			/**
			bitpos: [[0]]
			UART Interrupt Identification Register Bit 0(UIIR0)
			This bit is used to indicate a pending interrupt in either a hard-wired prioritized or a polled environment,with a logic 0 state.When the condition takes place,UIRR contents may be used as a pointer to the appropriate interrupt service routine.
			 */
			uint8_t uiir0:			1;
			/**
			bitpos: [[1]]
			UART Interrupt Identification Register Bit 1(UIIR1)
			These bits are used to identify the highest priority pending Interrupt.
			 */
			uint8_t uiir1:			1;
			/**
			bitpos: [[2]]
			UART Interrupt Identification Register Bit 2(UIIR2)
			These bits are used to identify the highest priority pending Interrupt.
			 */
			uint8_t uiir2:			1;
			/**
			bitpos: [[3]]
			UART Interrupt Identification Register Bit 3(UIIR3)
			In non-FIFO mode,this bit is logic 0.In the FIFO mode,this bit is set along with bit 2 when a time-out Interrupt is pending.
			 */
			uint8_t uiir3:			1;
			uint8_t reserved4:			2;
			/**
			bitpos: [[6]]
			UART Interrupt Identification Register Bit 6(UIIR6)
			Are set when UFCR(0)=1.
			 */
			uint8_t uiir6:			1;
			/**
			bitpos: [[7]]
			UART Interrupt Identification Register Bit 7(UIIR7)
			Are set when UFCR(0)=1.
			 */
			uint8_t uiir7:			1;
		};
		uint8_t val;
	} uiir;	/* REG_UART_BASE + 0x2 */

	union {
		struct {
			/**
			bitpos: [[1:0]]
			Word Length Select Bit 1,Bit 0 (WLS1,WLS0)
			Specify the number of bits in each serial character, encoded below:
			WLS1       WLS0          Word Length
			  0           0               5 bits
			  0           1               6 bits
			  1           0               7 bits
			  1           1               8 bits
			 */
			uint8_t wls:			2;
			/**
			bitpos: [[2]]
			Stop Bit Select (STB)
			Specifies the number of stop bits in each serial character,
			Summarized below:
			STB      Word Length    No.of Stop Bits
			0             -              1 bit
			1             5             1.5 bits
			1             6              2 bits
			1             7              2 bits
			1             8              2 bits
			Note:The receiver will ignore all stop bits beyond the first,regardless of the number used in transmission.
			 */
			uint8_t stb:			1;
			/**
			bitpos: [[3]]
			Parity Enable (PEN)
			A parity bit,between the last data word bit and stop bit.will be generated or checked (transmit or receive data)
			when this bit is high.
			 */
			uint8_t pen:			1;
			/**
			bitpos: [[4]]
			Even Parity Select (EPS)
			When parity is enabled (Parity Enable=1),EPS=0 selects odd parity,and EPS=1 selects even parity.
			 */
			uint8_t eps:			1;
			/**
			bitpos: [[5]]
			Stick Parity Bit (SP)
			When this bit and Parity Enable (PEN) bit are high at the same time,the parity bit is transmitted and then detected by the receiver.In opposite state,the parity bit is detected by bit Even Parity Select (EPS) bit to force the parity to a known state and to check the parity bit in a known state.
			 */
			uint8_t sp:			1;
			/**
			bitpos: [[6]]
			Break Control (BREAK)
			Forces the Serial Output (SOUT) to the spacing state (logic 0) by a logic 1,and this state will remain until a low level resets this bit,enabling the serial port to alert the terminal in a communication system.
			 */
			uint8_t break_ctrl:			1;
			/**
			bitpos: [[7]]
			Divisor Latch Access Bit (DLAB)
			Must be set high to access the Divisor Latches of the baud rate generator during read or write operations.It must be set low to access the Data Register (URBR and UTBR) or the Interrupt Enable Register.
			 */
			uint8_t dlab:			1;
		};
		uint8_t val;
	} ulcr;	/* REG_UART_BASE + 0x3 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Data Terminal Ready (DTR)
			Controls the Data Terminal Ready (DTR#),which is in an inverse logic state with this bit.This bit set to 0 will disable receive.
			 */
			uint8_t dtr:			1;
			/**
			bitpos: [[1]]
			Request To Send (RTS)
			Controls the Request to Send (RTS#) which is in an inverse logic state with this bit.This bit set to 0 will disable receive.
			 */
			uint8_t rts:			1;
			uint8_t reserved2:			1;
			/**
			bitpos: [[3]]
			OUT2
			This bit is the Output 2 bit and enables the serial port interrupt output by logic 1.
			 */
			uint8_t out2:			1;
			/**
			bitpos: [[4]]
			LOOP
			Provides a loop back feature for diagnostic test of the serial channel when it is set high.Serial Output (SOUT) is set to the Marking State Shift Register output Loops back into the Receiver Shift Register.All Modem Control inputs (CTS#,DSR#,RI# and DCD#) are disconnected;the four Modem Control inputs are forced to inactive high. The transmitted data are immediately received,allowing the processor to verify the transmitting and receiving data paths of the serial channel.
			 */
			uint8_t loop:			1;
			uint8_t reserved5:			3;
		};
		uint8_t val;
	} umcr;	/* REG_UART_BASE + 0x4 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Data Ready (DR)
			Data Ready (DR) bit logic “1”,which indicates a character has been received by URBR.Logic ‘0” indicates all data in the URBR or RCV FIFO have been read.
			 */
			uint8_t dr:			1;
			/**
			bitpos: [[1]]
			Overrun Error (OE)
			Overrun Error (OE) bit which indicates by a logic “1” that the URBR had been overwritten by the next character before it was read by the CPU.In the FIFO mode,the OE occurs when the FIFO is full and the next character has been completely received by the Shift Register.It will be reset when CPU reads the ULSR.
			 */
			uint8_t oe:			1;
			/**
			bitpos: [[2]]
			Parity Error (PE)
			Indicates the parity error (PE) with a logic “1”, representing that the received data character does not have the correct even or odd parity,as selected by bit 4 of ULCR (Even Parity Select).It will be reset to “0” whenever the CPU reads the ULSR.
			 */
			uint8_t pe:			1;
			/**
			bitpos: [[3]]
			Framing Error (FE)
			When this bit is logic 1,it indicates that the stop bit in the received character was not valid.It is reset low when the CPU reads the contents of ULSR.
			 */
			uint8_t fe:			1;
			/**
			bitpos: [[4]]
			Break Interrupt (BI)
			This bit indicates that the last received character was a break character.The break interrupt status bit will be asserted only when the last received character,parity bits and stop bits are all break bits.When any of these error conditions is detected (ULSR(1) to ULSR (4)),a Receiver Line Status interrupt (priority 1) will be produced in the UIIR,with the UIER(2) previously enabled.
			 */
			uint8_t bi:			1;
			/**
			bitpos: [[5]]
			Transmitter Holding Register Empty (THRE)
			This read only bit indicates that the UTBR is empty,and is ready to accept a new character for transmission.It is set high when a character is transferred from the THR into the Transmitter Shift Register,causing a priority 3 UIIR interrupt which is cleared by a read of UIIR.In the FIFO mode,it is set when the XMIT FIFO is empty,and is cleared when at least one byte is written to the XMIT FIFO.
			 */
			uint8_t thre:			1;
			/**
			bitpos: [[6]]
			Transmitter Empty (TEMT)
			This read only bit indicates that the Transmitter Holding Register and Transmitter Shift Register are both empty; otherwise,this bit is”0”.It has the same function in the FIFO mode.
			 */
			uint8_t temt:			1;
			/**
			bitpos: [[7]]
			Error In RCVR FIFO (ERF)
			In 16550 mode,this bit is always 0.In the FIFO mode,it is set high when there is at least one parity error,framing or break interrupt in the FIFO.This bit is cleared when the CPU reads ULSR,if there are no subsequent errors in the FIFO.
			 */
			uint8_t erf:			1;
		};
		uint8_t val;
	} ulsr;	/* REG_UART_BASE + 0x5 */

	union {
		struct {
			/**
			bitpos: [[0]]
			Delta Clear to Send (DCTS)
			This bit indicates that the CTS# input state to the serial channel has been changed since the last time it was read by the Host
			 */
			uint8_t dcts:			1;
			/**
			bitpos: [[1]]
			Delta Data Set Ready (DDSR)
			A logic “1” indicates that the DSR# input to the serial channel has changed the state since the last time it was read by the Host
			 */
			uint8_t ddsr:			1;
			/**
			bitpos: [[2]]
			Trailing Edge of Ring Indicator (TERI)
			Indicates that the RI input state to the serial channel has been changed from a low to high since the last time it was read by the Host.The change to logic 1 doesn’t activate the TERI.
			 */
			uint8_t teri:			1;
			/**
			bitpos: [[3]]
			Delta Data Carrier Detect (DDCD)
			Indicates that the DCD# input state has been changed since the last time it was read by the Host.
			 */
			uint8_t ddcd:			1;
			/**
			bitpos: [[4]]
			Clear to Send (CTS#)
			Indicates the complement of CTS# input. If the serial channel is in the loop mode (bit 4 of UMCR is 1),this bit is equivalent to RTS# in the UMCR
			 */
			uint8_t cts:			1;
			/**
			bitpos: [[5]]
			Data Set Ready (DSR#)
			Indicates the modem is ready to provide received data to the serial Channel receiver circuitry,If the serial channel is in the loop mode (bit 5 of UMCR is 1),this bit is equivalent to DTR# in the UMCR
			 */
			uint8_t dsr:			1;
			/**
			bitpos: [[6]]
			Ring Indicator (RI#)
			Indicates the complement to the RI# input.If bit 4 of UMCR is 1,this bit is Equivalent to OUT1 of the UMCR.
			 */
			uint8_t ri:			1;
			/**
			bitpos: [[7]]
			Data Carrier Detect (DCD#)
			Indicates the complement status of Data Carrier Detect input.If bit 4 of UMCR is 1,this bit is equivalent to OUT2 of the UMCR.
			 */
			uint8_t dcd:			1;
		};
		uint8_t val;
	} umsr;	/* REG_UART_BASE + 0x6 */
	/**
 UART Scratch Pad Register (USCR)
 This 8-bit register does not control the operation of UART in any way.It is intended as a scratch pad register to be used by programmers to Temporarily hold general-purpose data.
	 */
	uint8_t uscr; /* REG_UART_BASE + 0x7 */

	union {
		struct {
			/**
			bitpos: [[0]]
			IRDA EN
			  Set this bit to enable IrDA transfer.UART_TX and UART_RX pin act as the IrDA TX and RX pin.UART’s TX and RX signal is internal connected to SIR block for transmit and receive.
			 */
			uint8_t irda_en:			1;
			/**
			bitpos: [[1]]
			IRDA loop mode
			  Same as the UART loop mode.IrDA TX was connected to IrdA RX for test the internal function is ok.
			 */
			uint8_t irda_loop_mode:			1;
			/**
			bitpos: [[2]]
			IRDA_TX INV EN
			  Invert the IrDA TX signal
			 */
			uint8_t irda_tx_inv_en:			1;
			/**
			bitpos: [[3]]
			FIN_SEL
			  Set this bit the let frequency feed in ultra clock.Default the frequency is 1843K (UART 115.2k max).
			 */
			uint8_t fin_sel:			1;
			uint8_t reserved4:			4;
		};
		uint8_t val;
	} dev_ctrl;	/* REG_UART_BASE + 0x8 */

	union {
		struct {
			/**
			bitpos: [[2:0]]
			GLITCH_V (range from 0 to 6)
			  Define the start bit glitch value,on first negedge pulse,if counter value bigger than this,it will be considered as the glitch.Default value is 3.
			 */
			uint8_t glitch_v:			3;
			/**
			bitpos: [[3]]
			SUPPRESS_GLITCH_MODE
			  Set 1 to enable Suppress glitch mode (Sample 12 times each bit),Set 0 to use normal receiving mode (VALUE1_R and GLITCH_V only valid when this bit set to 1).
			 */
			uint8_t suppress_glitch_mode:			1;
			/**
			bitpos: [[7:4]]
			VALUE1 R (range from 0 to 11)
			  Define the signal sample count.The UART will sample one bit 7 times,if counter value bigger than the VALUE1_R,it would be accepted as value 1,or accepted as value 0.Default value is 4.
			 */
			uint8_t value1_r:			4;
		};
		uint8_t val;
	} rcvpr;	/* REG_UART_BASE + 0x9 */

	union {
		struct {
			/**
			bitpos: [[7:0]]
			STOP_LENGTH
			  This register sets the transmit delay between bytes according to CLK_EN,every 16 CLK_EN transmit one bit,so the default transmit delay between bytes is two bits.
			 */
			uint8_t stop_length:			8;
		};
		uint8_t val;
	} stop_length_reg;	/* REG_UART_BASE + 0xa */

	uint8_t reserved_b; /* REG_UART_BASE + 0xb */

	union {
		struct {
			/**
			bitpos: [[0]]
			New tx fifo trigger level int enable
			 */
			uint8_t tx_fifo_int_en:			1;
			/**
			bitpos: [[1]]
			New rx fifo trigger level int enable
			 */
			uint8_t rx_fifo_int_en:			1;
			uint8_t reserved2:			2;
			/**
			bitpos: [[4]]
			1: set old rx fifo trigger_level = 0
			 */
			uint8_t rcvr_fifo_disable:			1;
			uint8_t reserved5:			3;
		};
		uint8_t val;
	} ier1;	/* REG_UART_BASE + 0xc */

	uint8_t reserved_d; /* REG_UART_BASE + 0xd */

	union {
		struct {
			/**
			bitpos: [[0]]
			New tx fifo trigger level int status,write 1 to clear
			 */
			uint8_t tx_fifo_int_st:			1;
			/**
			bitpos: [[1]]
			New rx fifo trigger level int status,write 1 to clear
			 */
			uint8_t rx_fifo_int_st:			1;
			uint8_t reserved2:			6;
		};
		uint8_t val;
	} isr1;	/* REG_UART_BASE + 0xe */

	uint8_t reserved_f; /* REG_UART_BASE + 0xf */

	union {
		struct {
			/**
			bitpos: [[4:0]]
			NEW_TX_FIFO_TRIG_LEVEL: write this offset is tx fifo trigger level, default is 8byte.
			TX_FIFO_CNT: read this offset is tx fifo counter.
			 */
			uint8_t tx_fifocnt:			5;
			uint8_t reserved5:			3;
		};
		uint8_t val;
	} tx_fifocnt;	/* REG_UART_BASE + 0x10 */

	uint8_t reserved_11; /* REG_UART_BASE + 0x11 */

	union {
		struct {
			/**
			bitpos: [[4:0]]
			NEW_RX_FIFO_TRIG_LEVEL: write this offset is rx fifo trigger level, default is 8byte.
			RX_FIFO_CNT: read this offset is rx fifo counter.
			 */
			uint8_t rx_fifocnt:			5;
			uint8_t reserved5:			3;
		};
		uint8_t val;
	} rx_fifocnt;	/* REG_UART_BASE + 0x12 */

	union {
		struct {
			/**
			bitpos: [[3:0]]
			Sample_times_value: sample times value setting
			 */
			uint8_t sample_times_value:			4;
			/**
			bitpos: [[4]]
			sample_times_conf_en: enable samples times configure
			 */
			uint8_t sample_times_conf_en:			1;
			uint8_t reserved5:			3;
		};
		uint8_t val;
	} sample_times_conf;	/* REG_UART_BASE + 0x13 */

	union {
		struct {
			/**
			bitpos: [[5:0]]
			Addbit_len_for_sample: add bit length for sample
			 */
			uint8_t addbit_len:			6;
			uint8_t reserved6:			2;
		};
		uint8_t val;
	} addbit_len;	/* REG_UART_BASE + 0x14 */

} uart_reg_t;

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __UART_REG_STRUCT_H__ */
