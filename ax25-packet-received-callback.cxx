#ifdef QT_TEST_DRIVE
void MainWindow::ax25_kiss_packet_received(const unsigned char * kiss_frame, int kiss_frame_length)
#endif
{
ax25_unpacked_frame frame, response;
static unsigned char kiss_response[AX25_KISS_MAX_FRAME_LENGTH];
int kiss_response_length;

	ui->plainTextEditAX25->appendPlainText(QString("received: ") + decodeKissFrame(QByteArray((const char *) kiss_frame, kiss_frame_length)));
	memset(& response, 0, sizeof response);
	if (!unpack_ax25_kiss_frame(kiss_frame, kiss_frame_length, & frame))
	{
		ui->plainTextEditAX25->appendPlainText("unpacking ax25 frame failed!");
		return;
	}
	if (!frame.is_command_frame)
	{
		ui->plainTextEditAX25->appendPlainText("cannot yet handle response frames!");
		return;
	}
	memcpy(response.dest_addr, frame.src_addr, AX25_CALLSIGN_FIELD_SIZE);
	memcpy(response.src_addr, frame.dest_addr, AX25_CALLSIGN_FIELD_SIZE);
	response.dest_ssid = frame.src_ssid;
	response.src_ssid = frame.dest_ssid;
	response.is_command_frame = false;
	response.info_length = 0;

	switch (frame.frame_type)
	{
	case AX25_FRAME_TYPE_UNNUMBERED:
		switch (frame.uframe.type)
		{
		case UNN_DISCONNECT:
		case UNN_SET_ASYNC_BALANCED_MODE:
			response.frame_type = AX25_FRAME_TYPE_UNNUMBERED;
			response.uframe.poll_final_bit = frame.uframe.poll_final_bit;
			response.uframe.type = UNN_ACKNOWLEDGE;
			break;
		default:
			ui->plainTextEditAX25->appendPlainText("unhandled u-frame!");
			return;
			break;
		}
		break;
	case AX25_FRAME_TYPE_INFORMATION:
		response.frame_type = AX25_FRAME_TYPE_SUPERVISORY;
		response.sframe.poll_final_bit = frame.iframe.poll_bit;
		response.sframe.type = SUP_RECEIVER_READY;
		ax25.vr ++;
		ax25.vr &= 7;
		response.sframe.nr = ax25.vr;
		break;
	case AX25_FRAME_TYPE_SUPERVISORY:
		switch (frame.sframe.type)
		{
		case SUP_RECEIVER_READY:
			response.frame_type = AX25_FRAME_TYPE_SUPERVISORY;
			response.sframe.poll_final_bit = frame.sframe.poll_final_bit;
			response.sframe.type = SUP_RECEIVER_READY;
			break;
			default:
				ui->plainTextEditAX25->appendPlainText("unhandled u-frame!");
				return;
			break;
		}
	default:
		ui->plainTextEditAX25->appendPlainText("unhandled u-frame!");
		return;
		break;
	}

	kiss_response_length = pack_ax25_frame_into_kiss_frame(& response, & kiss_response);
	if (kiss_response_length == -1)
		ui->plainTextEditAX25->appendPlainText("could not pack response frame!");
	else
		ui->plainTextEditAX25->appendPlainText(QString("generated response: ") + decodeKissFrame(QByteArray((const char *) kiss_response, kiss_response_length)));

}

