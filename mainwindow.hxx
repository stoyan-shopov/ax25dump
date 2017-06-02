#ifndef MAINWINDOW_HXX
#define MAINWINDOW_HXX

#include <QMainWindow>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

enum
{
	KISS_FEND			=	/* frame end character */	0xc0,
	MINIMAL_FRAME_LENGTH_BYTES	=	/* kiss frame type byte */ 1 + /* address fields */ 14 + /* control byte */ 1,
	KISS_FRAME_TYPE_DATA		=	0,
	CALLSIGN_FIELD_SIZE		=	6,
	DEST_CALLSIGN_INDEX		=	1,
	DEST_SSID_INDEX			=	DEST_CALLSIGN_INDEX + 6,
	SRC_CALLSIGN_INDEX		=	DEST_SSID_INDEX + 1,
	SRC_SSID_INDEX			=	SRC_CALLSIGN_INDEX + 6,
	CONTROL_FIELD_INDEX		=	SRC_SSID_INDEX + 1,
	PID_FIELD_INDEX			=	CONTROL_FIELD_INDEX + 1,
	PID_NO_LAYER_3_PROTOCOL		=	0xf0,
};

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
	void decodeKissFrame(const QByteArray & frame);
	QByteArray s1_packet, s2_packet;
private slots:
	void s1Connected(void);
	void s2Connected(void);
	void s1ReadyRead(void);
	void s2ReadyRead(void);
};

#endif // MAINWINDOW_HXX
