#include "mainwindow.hxx"
#include "ui_mainwindow.h"

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

void MainWindow::decodeKissFrame(const QByteArray &frame)
{
QByteArray dest, src;
int i;
unsigned char dest_ssid, src_ssid, control;
QString s;

	if (frame.length() < MINIMAL_FRAME_LENGTH_BYTES)
	{
		if (frame.length())
			ui->plainTextEditDecodedFrames->appendPlainText("invalid frame - frame too short");
		return;
	}
	if (frame[0] & 0xf != KISS_FRAME_TYPE_DATA)
	{
		ui->plainTextEditDecodedFrames->appendPlainText("unsupported frame type - skipping");
		return;
	}
	dest = frame.mid(DEST_CALLSIGN_INDEX, CALLSIGN_FIELD_SIZE);
	dest_ssid = frame[DEST_SSID_INDEX];
	src = frame.mid(SRC_CALLSIGN_INDEX, CALLSIGN_FIELD_SIZE);
	src_ssid = frame[SRC_SSID_INDEX];
	control = frame[CONTROL_FIELD_INDEX];
	for (i = 0; i < dest.length(); dest[i] = (unsigned char) dest[i] >> 1, i ++);
	for (i = 0; i < src.length(); src[i] = (unsigned char) src[i] >> 1, i ++);
	s += QString::fromLocal8Bit(src) + " --> " + QString::fromLocal8Bit(dest);
	/* decode control byte */
	if (!(control & 1))
	{
		if (frame.length() <= PID_FIELD_INDEX + /* 2 bytes for the FCS field */ 2)
			s += "bad I frame length";
		else
		{
			s += QString(" (I frame): N(R) = %1, N(S) = %2, P/F = %3 ").arg(control >> 5).arg((control >> 1) & 3).arg((control & 4) ? 1 : 0);
			if ((unsigned char)frame[PID_FIELD_INDEX] != PID_NO_LAYER_3_PROTOCOL)
				s += QString("unsupported PID type - %1").arg((unsigned)(unsigned char) frame[PID_FIELD_INDEX]);
			else
			{
				s += "info: " + QString::fromLocal8Bit(frame.mid(PID_FIELD_INDEX + 1, frame.length() - PID_FIELD_INDEX - 1 - 2));
			}
		}
	}
	else if (!(control & 2))
	{
		unsigned char t = (control >> 2) & 3;
		s += QString(" (S frame): N(R) = %1, S = %2, P/F = %3 ").arg(control >> 5).arg(t).arg((control & 4) ? 1 : 0);
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
		s += QString(" (U frame): M = %1, P/F = %2 ").arg(m).arg((control & 4) ? 1 : 0);
		switch (m)
		{
			case UNN_ACKNOWLEDGE:			s += "UA"; break;
			case UNN_SET_ASYNC_BALANCED_MODE:	s += "SABM"; break;
			case UNN_DISCONNECT:			s += "DISC"; break;
			default: 				s += "UNKNOWN"; break;
		}
	}
	ui->plainTextEditDecodedFrames->appendPlainText(s);
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
		dump("s1", s1_packet), decodeKissFrame(s1_packet), s1_packet.clear();
	}
}

void MainWindow::s2ReadyRead()
{
auto s = s2.readAll();
	while (append(s2_packet, s))
	{
		s1.write(QByteArray(1, KISS_FEND) + s2_packet + QByteArray(1, KISS_FEND));
		dump("s2", s2_packet), decodeKissFrame(s2_packet), s2_packet.clear();
	}
}
