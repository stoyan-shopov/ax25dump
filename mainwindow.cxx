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
auto pos = from.indexOf(0xc0);

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
	dest = frame.mid(1, 6);
	dest_ssid = frame[1 + 6];
	src = frame.mid(1 + 7, 6);
	src_ssid = frame[1 + 7 + 6];
	control = frame[1 + 7 + 7];
	for (i = 0; i < dest.length(); dest[i] = (unsigned char) dest[i] >> 1, i ++);
	for (i = 0; i < src.length(); src[i] = (unsigned char) src[i] >> 1, i ++);
	s += QString::fromLocal8Bit(src) + " --> " + QString::fromLocal8Bit(dest);
	/* decode control byte */
	if (!(control & 1))
		s += QString(" (I frame): N(R) = %1, N(S) = %2, P/F = %3").arg(control >> 5).arg((control >> 1) & 3).arg((control & 4) ? 1 : 0);
	else if (!(control & 2))
		s += QString(" (I frame): N(R) = %1, S = %2, P/F = %3").arg(control >> 5).arg((control >> 1) & 3).arg((control & 4) ? 1 : 0);
	else
		s += QString(" (U frame): M = %1, P/F = %2").arg(((control >> 2) & 3) | ((control >> 3) & ~ 3)).arg((control & 4) ? 1 : 0);
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
	s2.write(s);
	//dump("s1xxx", s);
	while (append(s1_packet, s))
		dump("s1", s1_packet), decodeKissFrame(s1_packet), s1_packet.clear();
}

void MainWindow::s2ReadyRead()
{
auto s = s2.readAll();
	s1.write(s);
	while (append(s2_packet, s))
		dump("s2", s2_packet), decodeKissFrame(s2_packet), s2_packet.clear();
}
