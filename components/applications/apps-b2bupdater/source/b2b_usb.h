#ifndef __B2B_USB_H
#define __B2B_USB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <sys/poll.h>

#define USB_EP_0 0x00000000 // Endpoint 0
#define USB_CSRL0_TXRDY 0x00000002 // Transmit Packet Ready
#define USB_TXCSRL1_TXRDY 0x00000001 // Transmit Packet Ready
#define USB_O_CSRL0 0x00000102 // USB Control and Status Endpoint
#define USB_TRANS_IN 0x00000102 // Normal IN transaction
#define USB_O_RXCSRL1 0x00000116 // USB Receive Control and Status
#define USB_CSRL0_RXRDY 0x00000001 // Receive Packet Ready
#define USB_O_COUNT0 0x00000108 // USB Receive Byte Count Endpoint
#define USB_CSRL0_RXRDYC 0x00000040 // RXRDY Clear
#define USB_CSRL0_DATAEND 0x00000008 // Data End
#define USB_RXCSRL1_RXRDY 0x00000001 // Receive Packet Ready

#define HWREGB(x) (*((volatile unsigned char *)(x)))
#define HWREGH(x) (*((volatile unsigned short *)(x)))

#define USB_O_FIFO0 0x00000020 // USB FIFO Endpoint 0
#define INDEX_TO_USB_EP(x) ((x) << 4)
#define USB_EP_TO_INDEX(x) ((x) >> 4)
#define EP_OFFSET(Endpoint) (Endpoint - 0x10)

#define USB_O_TXIS 0x00000002 // USB Transmit Interrupt Status
#define USB_O_RXIS 0x00000004 // USB Receive Interrupt Status
#define USB_O_IS 0x0000000A // USB General Interrupt Status

extern unsigned int ulUsbBase;
extern unsigned int HCFlag;

uint32_t USBIntStatusEndpoint(uint32_t ulBase);
int USBEndpointDataPut(uint32_t ulBase, uint32_t ulEndpoint,
		       unsigned char *pucData, uint32_t ulSize);
int USBEndpointDataSend(uint32_t ulBase, uint32_t ulEndpoint,
			uint32_t ulTransType);
int USBEndpointDataGet(uint32_t ulBase, uint32_t ulEndpoint,
		       unsigned char *pucData, uint32_t *pulSize);
void USBDevEndpointDataAck(uint32_t ulBase, uint32_t ulEndpoint,
			   uint32_t bIsLastPacket);
void USBDevCommGetCmd(void);
void USBDevCommSendData(unsigned int percent);

extern unsigned int pFlashMax;
extern unsigned int pFlashAddr;
int get_data_form_brom(int flag, unsigned long param, uint32_t offsize,
		       void **buf, uint32_t *size);
int HBRomSendStatus(unsigned int param);

#endif
