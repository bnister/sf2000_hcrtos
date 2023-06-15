#ifndef __UART_REG_H__
#define __UART_REG_H__

#ifndef __KERNEL__
#ifdef __cplusplus
extern "c" {
#endif
#endif


#define __REG_UART_BASE		( REG_UART_BASE )


#define R_UART_URBR_UTBR		( __REG_UART_BASE + 0x0 )

/**
 bitpos: [7:0]
 UART Receiver Buffer Register (URBR)
 This register receives and holds the entering data.

 UART Transmitter Buffer Register (UTBR)
 This register holds and transmits the data via a non-accessible shift register.
 */
#define F_UART_0000_REV_TRANS_BUFFER 0xFF
#define F_UART_0000_REV_TRANS_BUFFER_M 0xFF
#define F_UART_0000_REV_TRANS_BUFFER_V 0xFF
#define F_UART_0000_REV_TRANS_BUFFER_S 0

#define R_UART_UIER		( __REG_UART_BASE + 0x1 )

/**
 bitpos: [0]
 Enable Received Data Available Interrupt (ERDVI)
 Sets this bit high to enable the Received Data Available Interrupt (and Time-out Interrupt in the FIFO mode)
 */
#define F_UART_0001_ERDVI 0x01
#define F_UART_0001_ERDVI_M 0x01
#define F_UART_0001_ERDVI_V 0x1
#define F_UART_0001_ERDVI_S 0
/**
 bitpos: [1]
 Enable Transmitter Holding Register Empty Interrupt(ETHREI)
 Sets this bit high to enable the Transmitter Holding Register Empty Interrupt .
 */
#define F_UART_0001_ETHREI 0x02
#define F_UART_0001_ETHREI_M 0x02
#define F_UART_0001_ETHREI_V 0x1
#define F_UART_0001_ETHREI_S 1
/**
 bitpos: [2]
 Enable Receiver Line Status Interrupt(ERLSI)
 Sets this bit high to enable the Receiver Line Status Interrupt ,which is caused when Overrun,Parity,Framing or Break occurs.
 */
#define F_UART_0001_ERLSI 0x04
#define F_UART_0001_ERLSI_M 0x04
#define F_UART_0001_ERLSI_V 0x1
#define F_UART_0001_ERLSI_S 2
/**
 bitpos: [3]
 Enable Modem Status Interrupt(EMSI)
 Sets this bit high to enable the Modem Status Interrupt when one of the Modem Status Registers changes its bit state
 */
#define F_UART_0001_EMSI 0x08
#define F_UART_0001_EMSI_M 0x08
#define F_UART_0001_EMSI_V 0x1
#define F_UART_0001_EMSI_S 3

#define R_UART_UIIR		( __REG_UART_BASE + 0x2 )

/**
 bitpos: [0]
 UART Interrupt Identification Register Bit 0(UIIR0)
 This bit is used to indicate a pending interrupt in either a hard-wired prioritized or a polled environment,with a logic 0 state.When the condition takes place,UIRR contents may be used as a pointer to the appropriate interrupt service routine.
 */
#define F_UART_0002_UIIR0 0x01
#define F_UART_0002_UIIR0_M 0x01
#define F_UART_0002_UIIR0_V 0x1
#define F_UART_0002_UIIR0_S 0
/**
 bitpos: [1]
 UART Interrupt Identification Register Bit 1(UIIR1)
 These bits are used to identify the highest priority pending Interrupt.
 */
#define F_UART_0002_UIIR1 0x02
#define F_UART_0002_UIIR1_M 0x02
#define F_UART_0002_UIIR1_V 0x1
#define F_UART_0002_UIIR1_S 1
/**
 bitpos: [2]
 UART Interrupt Identification Register Bit 2(UIIR2)
 These bits are used to identify the highest priority pending Interrupt.
 */
#define F_UART_0002_UIIR2 0x04
#define F_UART_0002_UIIR2_M 0x04
#define F_UART_0002_UIIR2_V 0x1
#define F_UART_0002_UIIR2_S 2
/**
 bitpos: [3]
 UART Interrupt Identification Register Bit 3(UIIR3)
 In non-FIFO mode,this bit is logic 0.In the FIFO mode,this bit is set along with bit 2 when a time-out Interrupt is pending.
 */
#define F_UART_0002_UIIR3 0x08
#define F_UART_0002_UIIR3_M 0x08
#define F_UART_0002_UIIR3_V 0x1
#define F_UART_0002_UIIR3_S 3
/**
 bitpos: [6]
 UART Interrupt Identification Register Bit 6(UIIR6)
 Are set when UFCR(0)=1.
 */
#define F_UART_0002_UIIR6 0x40
#define F_UART_0002_UIIR6_M 0x40
#define F_UART_0002_UIIR6_V 0x1
#define F_UART_0002_UIIR6_S 6
/**
 bitpos: [7]
 UART Interrupt Identification Register Bit 7(UIIR7)
 Are set when UFCR(0)=1.
 */
#define F_UART_0002_UIIR7 0x80
#define F_UART_0002_UIIR7_M 0x80
#define F_UART_0002_UIIR7_V 0x1
#define F_UART_0002_UIIR7_S 7

#define R_UART_ULCR		( __REG_UART_BASE + 0x3 )

/**
 bitpos: [1:0]
 Word Length Select Bit 1,Bit 0 (WLS1,WLS0)
 Specify the number of bits in each serial character, encoded below:
 WLS1       WLS0          Word Length
   0           0               5 bits
   0           1               6 bits
   1           0               7 bits
   1           1               8 bits
 */
#define F_UART_0003_WLS 0x03
#define F_UART_0003_WLS_M 0x03
#define F_UART_0003_WLS_V 0x3
#define F_UART_0003_WLS_S 0
/**
 bitpos: [2]
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
#define F_UART_0003_STB 0x04
#define F_UART_0003_STB_M 0x04
#define F_UART_0003_STB_V 0x1
#define F_UART_0003_STB_S 2
/**
 bitpos: [3]
 Parity Enable (PEN)
 A parity bit,between the last data word bit and stop bit.will be generated or checked (transmit or receive data)
 when this bit is high.
 */
#define F_UART_0003_PEN 0x08
#define F_UART_0003_PEN_M 0x08
#define F_UART_0003_PEN_V 0x1
#define F_UART_0003_PEN_S 3
/**
 bitpos: [4]
 Even Parity Select (EPS)
 When parity is enabled (Parity Enable=1),EPS=0 selects odd parity,and EPS=1 selects even parity.
 */
#define F_UART_0003_EPS 0x10
#define F_UART_0003_EPS_M 0x10
#define F_UART_0003_EPS_V 0x1
#define F_UART_0003_EPS_S 4
/**
 bitpos: [5]
 Stick Parity Bit (SP)
 When this bit and Parity Enable (PEN) bit are high at the same time,the parity bit is transmitted and then detected by the receiver.In opposite state,the parity bit is detected by bit Even Parity Select (EPS) bit to force the parity to a known state and to check the parity bit in a known state.
 */
#define F_UART_0003_SP 0x20
#define F_UART_0003_SP_M 0x20
#define F_UART_0003_SP_V 0x1
#define F_UART_0003_SP_S 5
/**
 bitpos: [6]
 Break Control (BREAK)
 Forces the Serial Output (SOUT) to the spacing state (logic 0) by a logic 1,and this state will remain until a low level resets this bit,enabling the serial port to alert the terminal in a communication system.
 */
#define F_UART_0003_BREAK_CTRL 0x40
#define F_UART_0003_BREAK_CTRL_M 0x40
#define F_UART_0003_BREAK_CTRL_V 0x1
#define F_UART_0003_BREAK_CTRL_S 6
/**
 bitpos: [7]
 Divisor Latch Access Bit (DLAB)
 Must be set high to access the Divisor Latches of the baud rate generator during read or write operations.It must be set low to access the Data Register (URBR and UTBR) or the Interrupt Enable Register.
 */
#define F_UART_0003_DLAB 0x80
#define F_UART_0003_DLAB_M 0x80
#define F_UART_0003_DLAB_V 0x1
#define F_UART_0003_DLAB_S 7

#define R_UART_UMCR		( __REG_UART_BASE + 0x4 )

/**
 bitpos: [0]
 Data Terminal Ready (DTR)
 Controls the Data Terminal Ready (DTR#),which is in an inverse logic state with this bit.This bit set to 0 will disable receive.
 */
#define F_UART_0004_DTR 0x01
#define F_UART_0004_DTR_M 0x01
#define F_UART_0004_DTR_V 0x1
#define F_UART_0004_DTR_S 0
/**
 bitpos: [1]
 Request To Send (RTS)
 Controls the Request to Send (RTS#) which is in an inverse logic state with this bit.This bit set to 0 will disable receive.
 */
#define F_UART_0004_RTS 0x02
#define F_UART_0004_RTS_M 0x02
#define F_UART_0004_RTS_V 0x1
#define F_UART_0004_RTS_S 1
/**
 bitpos: [3]
 OUT2
 This bit is the Output 2 bit and enables the serial port interrupt output by logic 1.
 */
#define F_UART_0004_OUT2 0x08
#define F_UART_0004_OUT2_M 0x08
#define F_UART_0004_OUT2_V 0x1
#define F_UART_0004_OUT2_S 3
/**
 bitpos: [4]
 LOOP
 Provides a loop back feature for diagnostic test of the serial channel when it is set high.Serial Output (SOUT) is set to the Marking State Shift Register output Loops back into the Receiver Shift Register.All Modem Control inputs (CTS#,DSR#,RI# and DCD#) are disconnected;the four Modem Control inputs are forced to inactive high. The transmitted data are immediately received,allowing the processor to verify the transmitting and receiving data paths of the serial channel.
 */
#define F_UART_0004_LOOP 0x10
#define F_UART_0004_LOOP_M 0x10
#define F_UART_0004_LOOP_V 0x1
#define F_UART_0004_LOOP_S 4

#define R_UART_ULSR		( __REG_UART_BASE + 0x5 )

/**
 bitpos: [0]
 Data Ready (DR)
 Data Ready (DR) bit logic “1”,which indicates a character has been received by URBR.Logic ‘0” indicates all data in the URBR or RCV FIFO have been read.
 */
#define F_UART_0005_DR 0x01
#define F_UART_0005_DR_M 0x01
#define F_UART_0005_DR_V 0x1
#define F_UART_0005_DR_S 0
/**
 bitpos: [1]
 Overrun Error (OE)
 Overrun Error (OE) bit which indicates by a logic “1” that the URBR had been overwritten by the next character before it was read by the CPU.In the FIFO mode,the OE occurs when the FIFO is full and the next character has been completely received by the Shift Register.It will be reset when CPU reads the ULSR.
 */
#define F_UART_0005_OE 0x02
#define F_UART_0005_OE_M 0x02
#define F_UART_0005_OE_V 0x1
#define F_UART_0005_OE_S 1
/**
 bitpos: [2]
 Parity Error (PE)
 Indicates the parity error (PE) with a logic “1”, representing that the received data character does not have the correct even or odd parity,as selected by bit 4 of ULCR (Even Parity Select).It will be reset to “0” whenever the CPU reads the ULSR.
 */
#define F_UART_0005_PE 0x04
#define F_UART_0005_PE_M 0x04
#define F_UART_0005_PE_V 0x1
#define F_UART_0005_PE_S 2
/**
 bitpos: [3]
 Framing Error (FE)
 When this bit is logic 1,it indicates that the stop bit in the received character was not valid.It is reset low when the CPU reads the contents of ULSR.
 */
#define F_UART_0005_FE 0x08
#define F_UART_0005_FE_M 0x08
#define F_UART_0005_FE_V 0x1
#define F_UART_0005_FE_S 3
/**
 bitpos: [4]
 Break Interrupt (BI)
 This bit indicates that the last received character was a break character.The break interrupt status bit will be asserted only when the last received character,parity bits and stop bits are all break bits.When any of these error conditions is detected (ULSR(1) to ULSR (4)),a Receiver Line Status interrupt (priority 1) will be produced in the UIIR,with the UIER(2) previously enabled.
 */
#define F_UART_0005_BI 0x10
#define F_UART_0005_BI_M 0x10
#define F_UART_0005_BI_V 0x1
#define F_UART_0005_BI_S 4
/**
 bitpos: [5]
 Transmitter Holding Register Empty (THRE)
 This read only bit indicates that the UTBR is empty,and is ready to accept a new character for transmission.It is set high when a character is transferred from the THR into the Transmitter Shift Register,causing a priority 3 UIIR interrupt which is cleared by a read of UIIR.In the FIFO mode,it is set when the XMIT FIFO is empty,and is cleared when at least one byte is written to the XMIT FIFO.
 */
#define F_UART_0005_THRE 0x20
#define F_UART_0005_THRE_M 0x20
#define F_UART_0005_THRE_V 0x1
#define F_UART_0005_THRE_S 5
/**
 bitpos: [6]
 Transmitter Empty (TEMT)
 This read only bit indicates that the Transmitter Holding Register and Transmitter Shift Register are both empty; otherwise,this bit is”0”.It has the same function in the FIFO mode.
 */
#define F_UART_0005_TEMT 0x40
#define F_UART_0005_TEMT_M 0x40
#define F_UART_0005_TEMT_V 0x1
#define F_UART_0005_TEMT_S 6
/**
 bitpos: [7]
 Error In RCVR FIFO (ERF)
 In 16550 mode,this bit is always 0.In the FIFO mode,it is set high when there is at least one parity error,framing or break interrupt in the FIFO.This bit is cleared when the CPU reads ULSR,if there are no subsequent errors in the FIFO.
 */
#define F_UART_0005_ERF 0x80
#define F_UART_0005_ERF_M 0x80
#define F_UART_0005_ERF_V 0x1
#define F_UART_0005_ERF_S 7

#define R_UART_UMSR		( __REG_UART_BASE + 0x6 )

/**
 bitpos: [0]
 Delta Clear to Send (DCTS)
 This bit indicates that the CTS# input state to the serial channel has been changed since the last time it was read by the Host
 */
#define F_UART_0006_DCTS 0x01
#define F_UART_0006_DCTS_M 0x01
#define F_UART_0006_DCTS_V 0x1
#define F_UART_0006_DCTS_S 0
/**
 bitpos: [1]
 Delta Data Set Ready (DDSR)
 A logic “1” indicates that the DSR# input to the serial channel has changed the state since the last time it was read by the Host
 */
#define F_UART_0006_DDSR 0x02
#define F_UART_0006_DDSR_M 0x02
#define F_UART_0006_DDSR_V 0x1
#define F_UART_0006_DDSR_S 1
/**
 bitpos: [2]
 Trailing Edge of Ring Indicator (TERI)
 Indicates that the RI input state to the serial channel has been changed from a low to high since the last time it was read by the Host.The change to logic 1 doesn’t activate the TERI.
 */
#define F_UART_0006_TERI 0x04
#define F_UART_0006_TERI_M 0x04
#define F_UART_0006_TERI_V 0x1
#define F_UART_0006_TERI_S 2
/**
 bitpos: [3]
 Delta Data Carrier Detect (DDCD)
 Indicates that the DCD# input state has been changed since the last time it was read by the Host.
 */
#define F_UART_0006_DDCD 0x08
#define F_UART_0006_DDCD_M 0x08
#define F_UART_0006_DDCD_V 0x1
#define F_UART_0006_DDCD_S 3
/**
 bitpos: [4]
 Clear to Send (CTS#)
 Indicates the complement of CTS# input. If the serial channel is in the loop mode (bit 4 of UMCR is 1),this bit is equivalent to RTS# in the UMCR
 */
#define F_UART_0006_CTS 0x10
#define F_UART_0006_CTS_M 0x10
#define F_UART_0006_CTS_V 0x1
#define F_UART_0006_CTS_S 4
/**
 bitpos: [5]
 Data Set Ready (DSR#)
 Indicates the modem is ready to provide received data to the serial Channel receiver circuitry,If the serial channel is in the loop mode (bit 5 of UMCR is 1),this bit is equivalent to DTR# in the UMCR
 */
#define F_UART_0006_DSR 0x20
#define F_UART_0006_DSR_M 0x20
#define F_UART_0006_DSR_V 0x1
#define F_UART_0006_DSR_S 5
/**
 bitpos: [6]
 Ring Indicator (RI#)
 Indicates the complement to the RI# input.If bit 4 of UMCR is 1,this bit is Equivalent to OUT1 of the UMCR.
 */
#define F_UART_0006_RI 0x40
#define F_UART_0006_RI_M 0x40
#define F_UART_0006_RI_V 0x1
#define F_UART_0006_RI_S 6
/**
 bitpos: [7]
 Data Carrier Detect (DCD#)
 Indicates the complement status of Data Carrier Detect input.If bit 4 of UMCR is 1,this bit is equivalent to OUT2 of the UMCR.
 */
#define F_UART_0006_DCD 0x80
#define F_UART_0006_DCD_M 0x80
#define F_UART_0006_DCD_V 0x1
#define F_UART_0006_DCD_S 7
/**
 bitpos: [7:0]
 UART Scratch Pad Register (USCR)
 This 8-bit register does not control the operation of UART in any way.It is intended as a scratch pad register to be used by programmers to Temporarily hold general-purpose data.
 */

#define R_UART_USCR		( __REG_UART_BASE + 0x7 )


#define R_UART_DEV_CTRL		( __REG_UART_BASE + 0x8 )

/**
 bitpos: [0]
 IRDA EN
   Set this bit to enable IrDA transfer.UART_TX and UART_RX pin act as the IrDA TX and RX pin.UART’s TX and RX signal is internal connected to SIR block for transmit and receive.
 */
#define F_UART_0008_IRDA_EN 0x01
#define F_UART_0008_IRDA_EN_M 0x01
#define F_UART_0008_IRDA_EN_V 0x1
#define F_UART_0008_IRDA_EN_S 0
/**
 bitpos: [1]
 IRDA loop mode
   Same as the UART loop mode.IrDA TX was connected to IrdA RX for test the internal function is ok.
 */
#define F_UART_0008_IRDA_LOOP_MODE 0x02
#define F_UART_0008_IRDA_LOOP_MODE_M 0x02
#define F_UART_0008_IRDA_LOOP_MODE_V 0x1
#define F_UART_0008_IRDA_LOOP_MODE_S 1
/**
 bitpos: [2]
 IRDA_TX INV EN
   Invert the IrDA TX signal
 */
#define F_UART_0008_IRDA_TX_INV_EN 0x04
#define F_UART_0008_IRDA_TX_INV_EN_M 0x04
#define F_UART_0008_IRDA_TX_INV_EN_V 0x1
#define F_UART_0008_IRDA_TX_INV_EN_S 2
/**
 bitpos: [3]
 FIN_SEL
   Set this bit the let frequency feed in ultra clock.Default the frequency is 1843K (UART 115.2k max).
 */
#define F_UART_0008_FIN_SEL 0x08
#define F_UART_0008_FIN_SEL_M 0x08
#define F_UART_0008_FIN_SEL_V 0x1
#define F_UART_0008_FIN_SEL_S 3

#define R_UART_RCVPR		( __REG_UART_BASE + 0x9 )

/**
 bitpos: [2:0]
 GLITCH_V (range from 0 to 6)
   Define the start bit glitch value,on first negedge pulse,if counter value bigger than this,it will be considered as the glitch.Default value is 3.
 */
#define F_UART_0009_GLITCH_V 0x07
#define F_UART_0009_GLITCH_V_M 0x07
#define F_UART_0009_GLITCH_V_V 0x7
#define F_UART_0009_GLITCH_V_S 0
/**
 bitpos: [3]
 SUPPRESS_GLITCH_MODE
   Set 1 to enable Suppress glitch mode (Sample 12 times each bit),Set 0 to use normal receiving mode (VALUE1_R and GLITCH_V only valid when this bit set to 1).
 */
#define F_UART_0009_SUPPRESS_GLITCH_MODE 0x08
#define F_UART_0009_SUPPRESS_GLITCH_MODE_M 0x08
#define F_UART_0009_SUPPRESS_GLITCH_MODE_V 0x1
#define F_UART_0009_SUPPRESS_GLITCH_MODE_S 3
/**
 bitpos: [7:4]
 VALUE1 R (range from 0 to 11)
   Define the signal sample count.The UART will sample one bit 7 times,if counter value bigger than the VALUE1_R,it would be accepted as value 1,or accepted as value 0.Default value is 4.
 */
#define F_UART_0009_VALUE1_R 0xF0
#define F_UART_0009_VALUE1_R_M 0xF0
#define F_UART_0009_VALUE1_R_V 0xF
#define F_UART_0009_VALUE1_R_S 4

#define R_UART_STOP_LENGTH_REG		( __REG_UART_BASE + 0xA )

/**
 bitpos: [7:0]
 STOP_LENGTH
   This register sets the transmit delay between bytes according to CLK_EN,every 16 CLK_EN transmit one bit,so the default transmit delay between bytes is two bits.
 */
#define F_UART_000A_STOP_LENGTH 0xFF
#define F_UART_000A_STOP_LENGTH_M 0xFF
#define F_UART_000A_STOP_LENGTH_V 0xFF
#define F_UART_000A_STOP_LENGTH_S 0

#define R_UART_IER1		( __REG_UART_BASE + 0xC )

/**
 bitpos: [0]
 New tx fifo trigger level int enable
 */
#define F_UART_000C_TX_FIFO_INT_EN 0x01
#define F_UART_000C_TX_FIFO_INT_EN_M 0x01
#define F_UART_000C_TX_FIFO_INT_EN_V 0x1
#define F_UART_000C_TX_FIFO_INT_EN_S 0
/**
 bitpos: [1]
 New rx fifo trigger level int enable
 */
#define F_UART_000C_RX_FIFO_INT_EN 0x02
#define F_UART_000C_RX_FIFO_INT_EN_M 0x02
#define F_UART_000C_RX_FIFO_INT_EN_V 0x1
#define F_UART_000C_RX_FIFO_INT_EN_S 1
/**
 bitpos: [4]
 1: set old rx fifo trigger_level = 0
 */
#define F_UART_000C_RCVR_FIFO_DISABLE 0x10
#define F_UART_000C_RCVR_FIFO_DISABLE_M 0x10
#define F_UART_000C_RCVR_FIFO_DISABLE_V 0x1
#define F_UART_000C_RCVR_FIFO_DISABLE_S 4

#define R_UART_ISR1		( __REG_UART_BASE + 0xE )

/**
 bitpos: [0]
 New tx fifo trigger level int status,write 1 to clear
 */
#define F_UART_000E_TX_FIFO_INT_ST 0x01
#define F_UART_000E_TX_FIFO_INT_ST_M 0x01
#define F_UART_000E_TX_FIFO_INT_ST_V 0x1
#define F_UART_000E_TX_FIFO_INT_ST_S 0
/**
 bitpos: [1]
 New rx fifo trigger level int status,write 1 to clear
 */
#define F_UART_000E_RX_FIFO_INT_ST 0x02
#define F_UART_000E_RX_FIFO_INT_ST_M 0x02
#define F_UART_000E_RX_FIFO_INT_ST_V 0x1
#define F_UART_000E_RX_FIFO_INT_ST_S 1

#define R_UART_TX_FIFOCNT		( __REG_UART_BASE + 0x10 )

/**
 bitpos: [4:0]
 NEW_TX_FIFO_TRIG_LEVEL: write this offset is tx fifo trigger level, default is 8byte.
 TX_FIFO_CNT: read this offset is tx fifo counter.
 */
#define F_UART_0010_TX_FIFOCNT 0x1F
#define F_UART_0010_TX_FIFOCNT_M 0x1F
#define F_UART_0010_TX_FIFOCNT_V 0x1F
#define F_UART_0010_TX_FIFOCNT_S 0

#define R_UART_RX_FIFOCNT		( __REG_UART_BASE + 0x12 )

/**
 bitpos: [4:0]
 NEW_RX_FIFO_TRIG_LEVEL: write this offset is rx fifo trigger level, default is 8byte.
 RX_FIFO_CNT: read this offset is rx fifo counter.
 */
#define F_UART_0012_RX_FIFOCNT 0x1F
#define F_UART_0012_RX_FIFOCNT_M 0x1F
#define F_UART_0012_RX_FIFOCNT_V 0x1F
#define F_UART_0012_RX_FIFOCNT_S 0

#define R_UART_SAMPLE_TIMES_CONF		( __REG_UART_BASE + 0x13 )

/**
 bitpos: [3:0]
 Sample_times_value: sample times value setting
 */
#define F_UART_0013_SAMPLE_TIMES_VALUE 0x0F
#define F_UART_0013_SAMPLE_TIMES_VALUE_M 0x0F
#define F_UART_0013_SAMPLE_TIMES_VALUE_V 0xF
#define F_UART_0013_SAMPLE_TIMES_VALUE_S 0
/**
 bitpos: [4]
 sample_times_conf_en: enable samples times configure
 */
#define F_UART_0013_SAMPLE_TIMES_CONF_EN 0x10
#define F_UART_0013_SAMPLE_TIMES_CONF_EN_M 0x10
#define F_UART_0013_SAMPLE_TIMES_CONF_EN_V 0x1
#define F_UART_0013_SAMPLE_TIMES_CONF_EN_S 4

#define R_UART_ADDBIT_LEN		( __REG_UART_BASE + 0x14 )

/**
 bitpos: [5:0]
 Addbit_len_for_sample: add bit length for sample
 */
#define F_UART_0014_ADDBIT_LEN 0x3F
#define F_UART_0014_ADDBIT_LEN_M 0x3F
#define F_UART_0014_ADDBIT_LEN_V 0x3F
#define F_UART_0014_ADDBIT_LEN_S 0

#ifndef __KERNEL__
#ifdef __cplusplus
}
#endif
#endif

#endif  /* __UART_REG_H__ */
