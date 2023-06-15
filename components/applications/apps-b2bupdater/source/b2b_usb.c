#include "b2b_usb.h"

uint32_t USBIntStatusEndpoint(uint32_t ulBase)
{
	uint32_t ulStatus;

	/* Check the arguments. */
	//ASSERT((ulBase == USB0_BASE)||(ulBase == USB1_BASE));

	/* Get the transmit interrupt status. */
	ulStatus = HWREGH(ulBase + USB_O_TXIS);

	ulStatus |= (HWREGH(ulBase + USB_O_RXIS) << 16);
	/* Return the combined interrupt status. */

#ifndef BRAD_USB
	/* Get the general interrupt status, these bits go into the upper 8 bits
	     * of the returned value. */
	ulStatus |= (HWREGB(ulBase + USB_O_IS));
#endif

	return (ulStatus);
}

int USBEndpointDataPut(uint32_t ulBase, uint32_t ulEndpoint,
		       unsigned char *pucData, uint32_t ulSize)
{
	uint32_t ulFIFO;
	unsigned char ucTxPktRdy;

	/* Get the bit position of TxPktRdy based on the endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ucTxPktRdy = USB_CSRL0_TXRDY;
	} else {
		ucTxPktRdy = USB_TXCSRL1_TXRDY;
	}

	/* Don't allow transmit of data if the TxPktRdy bit is already set. */
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy) {
		return (-1);
	}

	/* Calculate the FIFO address. */
	ulFIFO = ulBase + USB_O_FIFO0 + (ulEndpoint >> 2);

	/* Write the data to the FIFO. */
	for (; ulSize > 0; ulSize--) {
		HWREGB(ulFIFO) = *pucData++;
	}

	/* Success. */
	return (0);
}

int USBEndpointDataSend(uint32_t ulBase, uint32_t ulEndpoint,
			uint32_t ulTransType)
{
	uint32_t ulTxPktRdy;
	uint32_t ucTxPktRdy;

	/* Get the bit position of TxPktRdy based on the endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ulTxPktRdy = ulTransType & 0xff;
		ucTxPktRdy = USB_CSRL0_TXRDY;
	} else {
		ulTxPktRdy = (ulTransType >> 8) & 0xff;
		ucTxPktRdy = USB_TXCSRL1_TXRDY;
	}

	/* Don't allow transmit of data if the TxPktRdy bit is already set. */
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy) {
		return (-1);
	}

	/* Set TxPktRdy in order to send the data. */
	HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) = ulTxPktRdy;

	/* Success. */
	return (0);
}

int USBEndpointDataGet(uint32_t ulBase, uint32_t ulEndpoint,
		       unsigned char *pucData, uint32_t *pulSize)
{
	uint32_t ulRegister, ulByteCount, ulFIFO;

	/* Get the address of the receive status register to use, based on the
     * endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ulRegister = USB_O_CSRL0;
	} else {
		ulRegister = USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint);
	}

	/* Don't allow reading of data if the RxPktRdy bit is not set. */
	if ((HWREGH(ulBase + ulRegister) & USB_CSRL0_RXRDY) == 0) {
		/* Can't read the data because none is available. */
		*pulSize = 0;

		/* Return a failure since there is no data to read. */
		return (-1);
	}

	/* Get the byte count in the FIFO. */
	ulByteCount = HWREGH(ulBase + USB_O_COUNT0 + ulEndpoint);

	/* Determine how many bytes we will actually copy. */
	ulByteCount = (ulByteCount < *pulSize) ? ulByteCount : *pulSize;

	/* Return the number of bytes we are going to read. */
	*pulSize = ulByteCount;

	/* Calculate the FIFO address. */
	ulFIFO = ulBase + USB_O_FIFO0 + (ulEndpoint >> 2);

	/* Read the data out of the FIFO. */
	for (; ulByteCount > 0; ulByteCount--) {
		/* Read a byte at a time from the FIFO. */
		*pucData++ = HWREGB(ulFIFO);
	}

	/* Success. */
	return (0);
}

void USBDevEndpointDataAck(uint32_t ulBase, uint32_t ulEndpoint,
			   uint32_t bIsLastPacket)
{
	/* Determine which endpoint is being acked. */
	if (ulEndpoint == USB_EP_0) {
		/* Clear RxPktRdy, and optionally DataEnd, on endpoint zero. */
		HWREGB(ulBase + USB_O_CSRL0) =
			USB_CSRL0_RXRDYC |
			(bIsLastPacket ? USB_CSRL0_DATAEND : 0);
	} else {
		/* Clear RxPktRdy on all other endpoints. */
		HWREGB(ulBase + USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint)) &=
			~(USB_RXCSRL1_RXRDY);
	}
}

void USBDevCommGetCmd(void)
{
	uint32_t ulByteCount = 0x0;
	unsigned char PktComment[32];
	while (1) {
		ulByteCount = HWREGH(ulUsbBase + 0x108 + 0x20);
		if (ulByteCount >= 20) {
			USBEndpointDataGet(ulUsbBase, 0x20, PktComment,
					   &ulByteCount);
			USBDevEndpointDataAck(ulUsbBase, 0x20, false);
			if (ulByteCount > 0x0) {
			}
		}
	}
}

void USBDevCommSendData(unsigned int percent)
{
	unsigned int ulByteCount = 0x0;
	unsigned char pCtrlWRStatus[4] = { 0x01, 0x00, 0x97, 0x19 };
	pCtrlWRStatus[0] = percent;
	vTaskDelay(1);
	USBIntStatusEndpoint(ulUsbBase);
	USBEndpointDataPut(ulUsbBase, INDEX_TO_USB_EP(0x1), pCtrlWRStatus, 4);
	USBEndpointDataSend(ulUsbBase, INDEX_TO_USB_EP(0x1), USB_TRANS_IN);
}

int get_data_form_brom(int flag, unsigned long param, uint32_t offsize,
			      void **buf, uint32_t *size)
{
	if (HCFlag & 0x01) {
		USBDevCommSendData(param);
		return 0;
	}
	if (HCFlag & 0x02) {
		*((unsigned char *)0xB8818300) = param;
		vTaskDelay(10);
		return 0;
	}
	return 0;
}

int HBRomSendStatus(unsigned int param)
{
	if (HCFlag & 0x01) {
		USBDevCommSendData(param);
		vTaskDelay(10);
		return 0;
	}
	if (HCFlag & 0x02) {
		*((unsigned char *)0xB8818300) = param;
		vTaskDelay(10);
		return 0;
	}
	return 0;
}
