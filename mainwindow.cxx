#include <stdint.h>
#include "mainwindow.hxx"
#include "ui_mainwindow.h"

static struct ax25 ax25;

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


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(& s1, SIGNAL(connected()), this, SLOT(s1Connected()));
	connect(& s2, SIGNAL(connected()), this, SLOT(s2Connected()));
	connect(& s1, SIGNAL(readyRead()), this, SLOT(s1ReadyRead()));
	connect(& s2, SIGNAL(readyRead()), this, SLOT(s2ReadyRead()));
	s1.connectToHost("localhost", 5555);
	s2.connectToHost("localhost", 5556);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::dump(const QString &prompt, const QByteArray &data)
{
	if (data.length())
		ui->plainTextEdit->appendPlainText(prompt + ">" + data.toHex() + "[" + data + "]");
}

bool MainWindow::append(QByteArray &to, QByteArray &from)
{
auto pos = from.indexOf(KISS_FEND);

	if (pos == -1)
	{
		to += from;
		return false;
	}
	to += from.left(pos);
	from = from.mid(pos + 1);
	return true;
	
}

QString MainWindow::decodeKissFrame(const QByteArray &frame)
{
QByteArray dest, src;
int i;
unsigned char dest_ssid, src_ssid, control;
int command_or_response;
QString s;
int length = frame.length();

	if ((length = frame.length()) < AX25_KISS_MINIMAL_FRAME_LENGTH_BYTES)
	{
		if (length)
			return "invalid frame - frame too short";
		return "";
	}
	if (frame[0] & 0xf != KISS_FRAME_TYPE_DATA)
		return "unsupported frame type - skipping";
	dest = frame.mid(DEST_CALLSIGN_INDEX, AX25_CALLSIGN_FIELD_SIZE);
	dest_ssid = frame[DEST_SSID_INDEX];
	src = frame.mid(SRC_CALLSIGN_INDEX, AX25_CALLSIGN_FIELD_SIZE);
	src_ssid = frame[SRC_SSID_INDEX];
	control = frame[CONTROL_FIELD_INDEX];
	for (i = 0; i < dest.length(); dest[i] = (unsigned char) dest[i] >> 1, i ++);
	for (i = 0; i < src.length(); src[i] = (unsigned char) src[i] >> 1, i ++);
	command_or_response = ((dest_ssid >> 6) & 2) | ((src_ssid >> 7) & 1);
	if (command_or_response == AX25_COMMAND_FRAME)
		s = "[COMMAND]\t";
	else if (command_or_response == AX25_RESPONSE_FRAME)
		s = "[RESPONSE]\t";
	else
		s = "[???????]\t";

	s += QString::fromLocal8Bit(src) + QString("[SSID $%1]").arg(src_ssid, 2, 16, QChar('0')) + " --> " + QString::fromLocal8Bit(dest) + QString("[SSID $%1]").arg(dest_ssid, 2, 16, QChar('0'));
	/* decode control byte */
	if (!(control & 1))
	{
		if (length <= PID_FIELD_INDEX + /* 2 bytes for the FCS field */ 2)
			s += "bad I frame length";
		else
		{
			s += QString(" (I frame): N(R) = %1, N(S) = %2, P/F = %3 ").arg(control >> 5).arg((control >> 1) & 3).arg((control & (1 << 4)) ? 1 : 0);
			if ((unsigned char)frame[PID_FIELD_INDEX] != PID_NO_LAYER_3_PROTOCOL)
				s += QString("unsupported PID type - %1").arg((unsigned)(unsigned char) frame[PID_FIELD_INDEX]);
			else
			{
				s += "info: " + QString::fromLocal8Bit(frame.mid(PID_FIELD_INDEX + 1, length - PID_FIELD_INDEX - 1 - 2));
			}
		}
	}
	else if (!(control & 2))
	{
		unsigned char t = (control >> 2) & 3;
		s += QString(" (S frame): N(R) = %1, S = %2, P/F = %3 ").arg(control >> 5).arg(t).arg((control & (1 << 4)) ? 1 : 0);
		switch (t)
		{
			case SUP_RECEIVER_READY:	s += "RR"; break;
			case SUP_RECEIVER_NOT_READY:	s += "RNR"; break;
			case SUP_REJECT:		s += "REJ"; break;
			case SUP_SELECTIVE_REJECT:	s += "SREJ"; break;
		}
	}
	else
	{
		unsigned char m = ((control >> 2) & 3) | ((control >> 3) & ~ 3);
		s += QString(" (U frame): M = %1, P/F = %2 ").arg(m).arg((control & (1 << 4)) ? 1 : 0);
		switch (m)
		{
			case UNN_ACKNOWLEDGE:			s += "UA"; break;
			case UNN_SET_ASYNC_BALANCED_MODE:	s += "SABM"; break;
			case UNN_DISCONNECT:			s += "DISC"; break;
			default: 				s += "UNKNOWN"; break;
		}
	}
	s += QString("\tCRC $%1 $%2 ").arg((unsigned) (frame[length - 2] & 0xff), 2, 16).arg((unsigned) (frame[length - 1] & 0xff), 2, 16) + (crc((const unsigned char *) frame.constData(), length) ? "NOT OK" : "ok");
	struct ax25_unpacked_frame x[1];
	s += unpack_ax25_frame((const unsigned char *) frame.constData(), frame.length(), x) ? "\tframe unpacked successfully" : "frame unpack error";
	s.prepend("\n");
	s.prepend(frame.toHex());
	return s;
}

void MainWindow::s1Connected()
{
}

void MainWindow::s2Connected()
{
}

void MainWindow::s1ReadyRead()
{
auto s = s1.readAll();
	while (append(s1_packet, s))
	{
		s2.write(QByteArray(1, KISS_FEND) + s1_packet + QByteArray(1, KISS_FEND));
		dump("s1", s1_packet);
		auto s = decodeKissFrame(s1_packet);
		if (!s.isEmpty())
			ui->plainTextEditDecodedFrames->appendPlainText(s);
		s1_packet.clear();
	}
}

void MainWindow::s2ReadyRead()
{
auto s = s2.readAll();
	while (append(s2_packet, s))
	{
		s1.write(QByteArray(1, KISS_FEND) + s2_packet + QByteArray(1, KISS_FEND));
		dump("s2", s2_packet);
		auto s = decodeKissFrame(s2_packet);
		if (!s.isEmpty())
			ui->plainTextEditDecodedFrames->appendPlainText(s);
		s2_packet.clear();
	}
}

bool unpack_ax25_frame(const unsigned char * kiss_frame, int kiss_frame_length, struct ax25_unpacked_frame * frame)
{
int i;
unsigned char control;
	if (kiss_frame_length < AX25_KISS_MINIMAL_FRAME_LENGTH_BYTES || crc(kiss_frame, kiss_frame_length) || ! (* kiss_frame & 0x80) || (* kiss_frame & 0xf) != KISS_FRAME_TYPE_DATA)
		return false;
	memcpy(frame->dest_addr, frame + DEST_CALLSIGN_INDEX, sizeof frame->dest_addr);
	memcpy(frame->src_addr, frame + DEST_CALLSIGN_INDEX, sizeof frame->src_addr);
	for (i = 0; i < sizeof frame->dest_addr; i ++)
		frame->dest_addr[i] >>= 1;
	for (i = 0; i < sizeof frame->src_addr; i ++)
		frame->src_addr[i] >>= 1;
	frame->dest_ssid = (kiss_frame[DEST_CALLSIGN_INDEX] >> 1) & 15;
	frame->src_ssid = (kiss_frame[SRC_CALLSIGN_INDEX] >> 1) & 15;
	frame->is_command_frame = ((kiss_frame[DEST_CALLSIGN_INDEX] & 0x80) && !(kiss_frame[SRC_CALLSIGN_INDEX] & 0x80)) ? true : false;

	/* decode control byte */
	control = kiss_frame[CONTROL_FIELD_INDEX];
	if (!(control & 1))
	{
		frame->frame_type = AX25_FRAME_TYPE_INFORMATION;
		if (kiss_frame_length <= PID_FIELD_INDEX + /* 2 bytes for the FCS field */ 2)
			/* bad I frame length */
			return false;
		frame->iframe.pid = kiss_frame[PID_FIELD_INDEX];
		frame->iframe.nr = control >> 5;
		frame->iframe.ns = (control >> 1) & 7;
		frame->iframe.poll_bit = (control >> 4) & 1;
		frame->length = kiss_frame_length - PID_FIELD_INDEX - /* one byte for the pid field */ 1 - /* two bytes for the fcs field */ 2;
		memcpy(frame->info, kiss_frame + PID_FIELD_INDEX + 1, frame->length);
		/* supervisory frames should not contain data */
		if (frame->length = kiss_frame_length - PID_FIELD_INDEX - /* two bytes for the fcs field */ 2)
			return false;
	}
	else if (!(control & 2))
	{
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
		frame->length = kiss_frame_length - PID_FIELD_INDEX - /* two bytes for the fcs field */ 2;
		memcpy(frame->info, kiss_frame + PID_FIELD_INDEX + 1, frame->length);
	}
	return true;
}

/* returns the length of the kiss frame constructed, -1 on error */
int pack_ax25_frame_into_kiss_frame(const ax25_unpacked_frame *frame, unsigned char (*kiss_buffer)[AX25_KISS_MAX_FRAME_LENGTH])
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
			* p ++ = (ax25.vr << 5) | (frame->iframe.poll_bit << 4) | (frame->sframe.type << 2) | 1;
			break;
		case AX25_FRAME_TYPE_UNNUMBERED:
			/* construct control field */
			* p ++ = (ax25.vr << 5) | (frame->iframe.poll_bit << 4) | (frame->sframe.type << 2) | 1;
	}
	fcs = crc(* kiss_buffer, p - * kiss_buffer);
	* p ++ = fcs;
	* p ++ = fcs >> 8;
	return p - * kiss_buffer;
}

void ax25_kiss_packet_received(const unsigned char * kiss_frame, int kiss_frame_length)
{
}

