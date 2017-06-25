#ifndef AX25_H
#define AX25_H

#include <stdint.h>
#include <stdbool.h>

enum
{
	AX25_MAX_INFO_LENGTH		=	256,
	KISS_FEND			=	/* frame end character */	0xc0,
	AX25_KISS_MINIMAL_FRAME_LENGTH_BYTES	=	/* kiss frame type byte */ 1 + /* address fields */ 14 + /* control byte */ 1 + /* frame check sequence */ 2,
	KISS_FRAME_TYPE_DATA		=	0,
	AX25_CALLSIGN_FIELD_SIZE	=	6,
	DEST_CALLSIGN_INDEX		=	1,
	DEST_SSID_INDEX			=	DEST_CALLSIGN_INDEX + 6,
	SRC_CALLSIGN_INDEX		=	DEST_SSID_INDEX + 1,
	SRC_SSID_INDEX			=	SRC_CALLSIGN_INDEX + 6,
	AX25_SSID_UNUSED_BITS_MASK	=	0x60,
	CONTROL_FIELD_INDEX		=	SRC_SSID_INDEX + 1,
	PID_FIELD_INDEX			=	CONTROL_FIELD_INDEX + 1,
	PID_NO_LAYER_3_PROTOCOL		=	0xf0,
	/* ssid-specified command or response frame */
	AX25_COMMAND_FRAME		=	2,
	AX25_RESPONSE_FRAME		=	1,
	AX25_KISS_MAX_FRAME_LENGTH	=	AX25_KISS_MINIMAL_FRAME_LENGTH_BYTES + /* 1 byte for the pid field */ 1 + AX25_MAX_INFO_LENGTH,
};

enum AX25_UNNUMBERED_FRAME_TYPE_ENUM /* unnumbered frame control field types */
{
	/* UA */	UNN_ACKNOWLEDGE			=	12,
	/* SABM */	UNN_SET_ASYNC_BALANCED_MODE	=	7,
	/* DISC */	UNN_DISCONNECT			=	8,
};

enum AX25_SUPERVISORY_FRAME_TYPE_ENUM /* supervisory frame field types */
{
	/* RR */	SUP_RECEIVER_READY		=	0,
	/* RNR */	SUP_RECEIVER_NOT_READY		=	1,
	/* REJ */	SUP_REJECT			=	2,
	/* SREJ */	SUP_SELECTIVE_REJECT		=	3,
};

enum AX25_FRAME_TYPE_ENUM
{
	AX25_FRAME_TYPE_INFORMATION,
	AX25_FRAME_TYPE_UNNUMBERED,
	AX25_FRAME_TYPE_SUPERVISORY,
};

struct ax25_unpacked_frame
{
	unsigned char dest_addr[AX25_CALLSIGN_FIELD_SIZE], src_addr[AX25_CALLSIGN_FIELD_SIZE];
	unsigned char dest_ssid, src_ssid;
	/* if true - the frame is a command frame, otherwise - a response frame */
	bool is_command_frame;
	enum AX25_FRAME_TYPE_ENUM frame_type;
	union
	{
		/* information frame control field data */
		struct
		{
			unsigned char	pid, nr, ns;
			unsigned char	poll_bit;
		}
		iframe;
		/* supervisory frame control field data */
		struct
		{
			unsigned char	nr;
			unsigned char	poll_final_bit;
			enum AX25_SUPERVISORY_FRAME_TYPE_ENUM	type;
		}
		sframe;
		/* unnumbered frame control field data */
		struct
		{
			unsigned char	poll_final_bit;
			enum AX25_UNNUMBERED_FRAME_TYPE_ENUM	type;
		}
		uframe;
	};
	/* information data in the frame */
	struct
	{
		int info_length;
		unsigned char info[AX25_MAX_INFO_LENGTH];
	};
};

struct ax25
{
	enum
	{
		STATE_DISCONNECTED	=	0,
		STATE_AWAITING_CONNECTION,
		STATE_AWAITING_RELEASE,
		STATE_CONNECTED,
		STATE_TIMER_RECOVERY,
	}
	state;
	unsigned char vs, va, vr;
};

uint16_t crc(const unsigned char * p, int len);
bool unpack_ax25_kiss_frame(const unsigned char * kiss_frame, int kiss_frame_length, struct ax25_unpacked_frame * frame);
/* returns the length of the kiss frame constructed, -1 on error */
int pack_ax25_frame_into_kiss_frame(const struct ax25_unpacked_frame * frame, unsigned char (* kiss_buffer)[AX25_KISS_MAX_FRAME_LENGTH]);

#endif // AX25_H
