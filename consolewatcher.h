#ifndef CONSOLEWATCHER_H
#define CONSOLEWATCHER_H

#include <QObject>
#include <QTextStream>

class QSocketNotifier;

class ConsoleWatcher : public QObject
{
	Q_OBJECT
public:

	static const QString exit_cmd;
	static const QString set_folder_cmd;
	static const QString set_camera_cmd;
	static const QString start_record_cmd;
	static const QString record_cmd;
	static const QString stop_save_cmd;
	static const QString record_interval_cmd;
	static const QString stop_record_cmd;
	static const QString export_record_cmd;
	static const QString ir_toggle_cmd;
	static const QString list_cams_cmd;
	static const QString list_connections_cmd;
	static const QString remote_connect_cmd;
	static const QString remote_ping_cmd;
	static const QString remote_disconnect_cmd;
	static const QString time_cmd;
	static const QString config_time_cmd;
	static const QString batch_cmd;
	static const QString tcp_timing_cmd;
	static const QString wait_cmd;
	static const QString help_cmd;

	explicit ConsoleWatcher(QObject *parent = nullptr);

	void batchRun(QString scriptPath);

	void run();
	void stop();

Q_SIGNALS:

	void exitTriggered(int status = 0);
	void setFolderTriggered(QString folder);
	void setCameraTriggered(int camRow);
	void startRecordSessionTriggered();
	void startRecordTriggered(int camRow);
	void saveImgsTriggered(int nImgs);
	void saveImgsContinuousTriggered();
	void stopSaveImgsTriggered();
	void saveImgsIntervalTriggered(int nImgs, int msec);
	void stopRecordTriggered();
	void exportRecordTriggered();
	void setIrPatternTriggered(bool on);
	void listCamerasTriggered();
	void listConnectionsTriggered();
	void remoteConnectionTriggered(QString host);
	void remotePingTriggered(int remoteRow);
	void remoteDisconnectionTriggered(QString host);
	void timeTriggered();
	void configTimeTriggered(QString timeServerAddr, quint16 port);
	void tcpTimingTriggered(bool enabled);
	void sleepTrigger(uint ms);
	void helpTriggered();
	void InvalidTriggered(QString cmd);

protected:

	void readCommand();
	void runCommand(QString line);
	void printHelp();

	bool isExit(QString cmd);

	QSocketNotifier *_notifier;
	QTextStream _out;
	QTextStream _in;

};

#endif // CONSOLEWATCHER_H
