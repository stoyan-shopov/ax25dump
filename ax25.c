#include <string.h>
#include "ax25.h"

enum
{
	CRC_INIT	=	0,
	CRC_POLY	=	0xa001,
};

uint16_t crc(const unsigned char * p, int len)
{
uint16_t c = CRC_INIT;
unsigned char d;
int i;

	while (len --)
	{
		d = c ^ * p ++;
		c >>= 8;
		for (i = 0; i < 8; i ++)
			if (d & (1 << i))
				c ^= CRC_POLY >> (7 - i), d ^= CRC_POLY << (i + 1);

	}
	return c;
}

bool unpack_ax25_kiss_frame(const unsigned char * kiss_frame, int kiss_frame_length, struct ax25_unpacked_frame * frame)
{
int i;
unsigned char control;
	if (kiss_frame_length < AX25_KISS_MINIMAL_FRAME_LENGTH_BYTES || crc(kiss_frame, kiss_frame_length) || ! (* kiss_frame & 0x80) || (* kiss_frame & 0xf) != KISS_FRAME_TYPE_DATA)
	{
		//qDebug() << "ax25 kiss frame too short";
		return false;
	}
	memcpy(frame->dest_addr, kiss_frame + DEST_CALLSIGN_INDEX, sizeof frame->dest_addr);
	memcpy(frame->src_addr, kiss_frame + SRC_CALLSIGN_INDEX, sizeof frame->src_addr);
	for (i = 0; i < sizeof frame->dest_addr; i ++)
		frame->dest_addr[i] >>= 1;
	for (i = 0; i < sizeof frame->src_addr; i ++)
		frame->src_addr[i] >>= 1;
	frame->dest_ssid = (kiss_frame[DEST_SSID_INDEX] >> 1) & 15;
	frame->src_ssid = (kiss_frame[SRC_SSID_INDEX] >> 1) & 15;
	frame->is_command_frame = ((kiss_frame[DEST_SSID_INDEX] & 0x80) && !(kiss_frame[SRC_SSID_INDEX] & 0x80)) ? true : false;

	/* decode control byte */
	control = kiss_frame[CONTROL_FIELD_INDEX];
	if (!(control & 1))
	{
		frame->frame_type = AX25_FRAME_TYPE_INFORMATION;
		if (kiss_frame_length <= PID_FIELD_INDEX + /* 2 bytes for the FCS field */ 2)
		{
			/* bad I frame length */
			//qDebug() << "ax25 I frame too short";
			return false;
		}
		frame->iframe.pid = kiss_frame[PID_FIELD_INDEX];
		frame->iframe.nr = control >> 5;
		frame->iframe.ns = (control >> 1) & 7;
		frame->iframe.poll_bit = (control >> 4) & 1;
		frame->info_length = kiss_frame_length - PID_FIELD_INDEX - /* one byte for the pid field */ 1 - /* two bytes for the fcs field */ 2;
		memcpy(frame->info, kiss_frame + PID_FIELD_INDEX + 1, frame->info_length);
	}
	else if (!(control & 2))
	{
		/* supervisory frames should not contain data */
		if (frame->info_length = kiss_frame_length - PID_FIELD_INDEX - /* two bytes for the fcs field */ 2)
		{
			//qDebug() << "bad ax25 S frame";
			return false;
		}
		frame->frame_type = AX25_FRAME_TYPE_SUPERVISORY;
		frame->sframe.nr = control >> 5;
		frame->sframe.type = (enum AX25_SUPERVISORY_FRAME_TYPE_ENUM) ((control >> 2) & 3);
		frame->sframe.poll_final_bit = (control >> 4) & 1;
	}
	else
	{
		frame->frame_type = AX25_FRAME_TYPE_UNNUMBERED;
		frame->uframe.type = (enum AX25_UNNUMBERED_FRAME_TYPE_ENUM) (((control >> 2) & 3) | ((control >> 3) & ~ 3));
		frame->uframe.poll_final_bit = (control >> 4) & 1;
		frame->info_length = kiss_frame_length - PID_FIELD_INDEX - /* two bytes for the fcs field */ 2;
		memcpy(frame->info, kiss_frame + PID_FIELD_INDEX + 1, frame->info_length);
	}
	return true;
}

/* returns the length of the kiss frame constructed, -1 on error */
int pack_ax25_frame_into_kiss_frame(const struct ax25_unpacked_frame *frame, unsigned char (*kiss_buffer)[AX25_KISS_MAX_FRAME_LENGTH])
{
unsigned char * p = * kiss_buffer;
int i;
uint16_t fcs;
	/* put kiss byte */
	* p ++ = 0x80;
	/* assemble destination and source addresses - callsigns and ssids */
	for (i = 0; i < AX25_CALLSIGN_FIELD_SIZE; i ++)
		* p ++ = frame->dest_addr[i] << 1;
	* p ++ = (frame->dest_ssid << 1) | AX25_SSID_UNUSED_BITS_MASK | (frame->is_command_frame ? 0x80 : 0);
	for (i = 0; i < AX25_CALLSIGN_FIELD_SIZE; i ++)
		* p ++ = frame->src_addr[i] << 1;
	* p ++ = (frame->src_ssid << 1) | AX25_SSID_UNUSED_BITS_MASK | (frame->is_command_frame ? 0 : 0x80) | /* address terminator bit */ 1;
	switch (frame->frame_type)
	{
		default:
			return -1;
		case AX25_FRAME_TYPE_INFORMATION:
			/* construct control field */
// to be done
			return -1;
			break;
		case AX25_FRAME_TYPE_SUPERVISORY:
			/* construct control field */
			* p ++ = (frame->sframe.nr << 5) | (frame->sframe.poll_final_bit << 4) | (frame->sframe.type << 2) | 1;
			break;
		case AX25_FRAME_TYPE_UNNUMBERED:
			/* construct control field */
			* p ++ = (frame->uframe.poll_final_bit << 4)
				| ((frame->uframe.type << 3) & 0xe0) | ((frame->uframe.type << 2) & 0x0c) | 3;
	}
	fcs = crc(* kiss_buffer, p - * kiss_buffer);
	* p ++ = fcs;
	* p ++ = fcs >> 8;
	return p - * kiss_buffer;
}

#include "ax25-packet-received-callback.cxx"