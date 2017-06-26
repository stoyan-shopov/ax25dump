#include <QMessageBox>
#include <stdint.h>
#include "mainwindow.hxx"
#include "ui_mainwindow.h"

struct ax25 ax25;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	connect(& s1, SIGNAL(connected()), this, SLOT(s1Connected()));
	connect(& s2, SIGNAL(connected()), this, SLOT(s2Connected()));
	connect(& s1, SIGNAL(readyRead()), this, SLOT(s1ReadyRead()));
	connect(& s2, SIGNAL(readyRead()), this, SLOT(s2ReadyRead()));
	connect(& serial_port, SIGNAL(readyRead()), this, SLOT(serialPortReadyRead()));
	serial_port.setPortName(SERIAL_PORT_NAME);
	if (!serial_port.open(QIODevice::ReadWrite))
		QMessageBox::warning(0, "error opening serial port",
			QString("could not open serial port ") + serial_port.portName()
				     + "\n\nserial communication will be unavailable");
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
	s += unpack_ax25_kiss_frame((const unsigned char *) frame.constData(), frame.length(), x) ? "\tframe unpacked successfully" : "frame unpack error";
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
QByteArray kiss_frame;
	while (append(s1_packet, s))
	{
		s2.write(kiss_frame = QByteArray(1, KISS_FEND) + s1_packet + QByteArray(1, KISS_FEND));
		if (serial_port.isOpen())
			serial_port.write(kiss_frame);
		dump("s1", s1_packet);
		auto s = decodeKissFrame(s1_packet);
		if (!s.isEmpty())
			ui->plainTextEditDecodedFrames->appendPlainText(s);
		if (s1_packet.length())
			ax25_kiss_packet_received((unsigned char *) s1_packet.constData(), s1_packet.length());
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

void MainWindow::serialPortReadyRead()
{
}

void ax25_kiss_response_ready_callback(const char * kiss_response, int kiss_response_length)
{
}

#include "ax25-packet-received-callback.cxx"
