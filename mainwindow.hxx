#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

extern "C"
{
#include "ax25.h"
}

#include <QMainWindow>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	QTcpSocket s1, s2;
	
private:
	Ui::MainWindow *ui;
	void dump(const QString & prompt, const QByteArray & data);
	bool append(QByteArray & to, QByteArray & from);
	QString decodeKissFrame(const QByteArray & frame);
	QByteArray s1_packet, s2_packet;
	void ax25_kiss_packet_received(const unsigned char * kiss_frame, int kiss_frame_length);
private slots:
	void s1Connected(void);
	void s2Connected(void);
	void s1ReadyRead(void);
	void s2ReadyRead(void);
};

#endif // MAINWINDOW_HXX
