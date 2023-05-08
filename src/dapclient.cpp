/* Copyright (C) 2023 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>

#include "dapclient.hpp"

DAPClient::DAPClient()
{
}

DAPClient::~DAPClient()
{
}

bool DAPClient::connected()
{
	return process && process->state() != QProcess::NotRunning;
}

void DAPClient::initialize(const QString &program, const QStringList &arguments,
		const QString &workingDirectory)
{
	if (state != DS_NOT_STARTED) {
		qDebug() << "unexpected debugger state at initialization:" << state;
		state = DS_NOT_STARTED;
	}
	if (process) {
		if (process->state() != QProcess::NotRunning)
			qDebug() << "killing running process";
		delete process;
	}
	process = new QProcess(this);
	process->setWorkingDirectory(workingDirectory);
	process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
	connect(process, &QProcess::readyReadStandardOutput, this, &DAPClient::readInput);
	connect(process, &QProcess::started, [this]{
		QJsonObject args = { { "adapterId", "xsystem4" } };
		sendRequest("initialize", args);
		state = DS_INITIALIZING;
	});
	connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			[this](int exitCode, QProcess::ExitStatus exitStatus) {
		if (exitStatus == QProcess::CrashExit) {
			// XXX: we don't emit terminateFinished here to prevent infinite
			//      crash-restart loop
			// TODO: should be some way to reinitialize manually in case of crash
			emit errorOccurred("xsystem4 process crashed");
		} else {
			emit terminateFinished();
		}
	});
	connect(process, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart)
			state = DS_NOT_STARTED;
		qDebug() << "error occurred:" << error;
	});
	process->start(program, arguments);
}

void DAPClient::launch()
{
	if (state == DS_CONFIGURING) {
		sendRequest("configurationDone");
		sendRequest("launch");
	} else if (state == DS_PAUSED) {
		sendRequest("continue");
	}
}

void DAPClient::pause()
{
	QJsonObject args { { "threadId", 0 } };
	sendRequest("pause", args);
}

void DAPClient::terminate()
{
	sendRequest("disconnect");
	// FIXME: handle unresponsive xsystem4 process
}

void DAPClient::kill(int msec)
{
	if (!process)
		return;
	sendRequest("disconnect");
	if (!process->waitForFinished(msec)) {
		process->kill();
	}
}

void DAPClient::next()
{
	QJsonObject args { { "threadId", 0 } };
	sendRequest("next", args);
}

void DAPClient::stepIn()
{
	QJsonObject args { { "threadId", 0 } };
	sendRequest("stepIn", args);
}

void DAPClient::stepOut()
{
	QJsonObject args { { "threadId", 0 } };
	sendRequest("stepOut", args);
}

int DAPClient::requestStackTrace()
{
	QJsonObject args { { "threadId", 0 } };
	return sendRequest("stackTrace", args);
}

int DAPClient::requestScopes(int frameId)
{
	QJsonObject args { { "frameId", frameId } };
	return sendRequest("scopes", args);
}

int DAPClient::requestVariables(int variablesReference)
{
	QJsonObject args { { "variablesReference", variablesReference } };
	return sendRequest("variables", args);
}

int DAPClient::setInstructionBreakpoints(QSet<uint32_t> locations)
{
	QJsonArray breakpoints;
	for (uint32_t location : locations) {
		QJsonObject bp { { "instructionReference", QString::number(location, 16) } };
		breakpoints.push_back(bp);
	}
	QJsonObject args { { "breakpoints", breakpoints } };
	return sendRequest("setInstructionBreakpoints", args);
}

void DAPClient::handleEvent(QJsonObject &event)
{
	QString evtype = event["event"].toString();
	if (evtype == "output") {
		QJsonObject body = event["body"].toObject();
		QString category = body["category"].toString();
		QString output = body["output"].toString();
		emit outputReceived(category, output);
	} else if (evtype == "initialized") {
		state = DS_CONFIGURING;
		emit initialized();
	} else if (evtype == "stopped") {
		state = DS_PAUSED;
		emit paused();
	} else if (evtype == "terminated") {
		state = DS_NOT_STARTED;
		emit terminated();
	} else {
		qDebug() << "Unhandled event type: " << evtype;
	}
}

void DAPClient::handleResponse(QJsonObject &response)
{
	if (!response["success"].toBool()) {
		qDebug() << response["command"].toString() << " request failed!";
		return;
	}

	int reqId = response["request_seq"].toInt();
	QString cmd = response["command"].toString();
	if (cmd == "launch") {
		state = DS_RUNNING;
		emit launched();
	} else if (cmd == "continue") {
		state = DS_RUNNING;
		emit continued();
	} else if (cmd == "stackTrace") {
		QJsonArray jsonFrames = response["body"].toObject()["stackFrames"].toArray();
		if (jsonFrames.size() == 0) {
			qDebug() << "stackTrace returned 0 frames";
		}
		QVector<StackFrame> frames(jsonFrames.size());
		for (int i = 0; i < jsonFrames.size(); i++) {
			QJsonObject frame = jsonFrames[i].toObject();
			QString ipStr = frame["instructionPointerReference"].toString();
			frames[i] = {
				.id = frame["id"].toInt(),
				.name = frame["name"].toString(),
				.address = ipStr.toInt(nullptr, 16)
			};
		}
		emit stackTraceReceived(reqId, frames);
	} else if (cmd == "scopes") {
		QJsonArray jsonScopes = response["body"].toObject()["scopes"].toArray();
		if (jsonScopes.size() == 0) {
			qDebug() << "scopes returned 0 scopes";
		}

		QVector<Scope> scopes(jsonScopes.size());
		for (int i = 0; i < jsonScopes.size(); i++) {
			QJsonObject scope = jsonScopes[i].toObject();
			scopes[i] = {
				.name = scope["name"].toString(),
				.presentationHint = scope["presentationHint"].toString(),
				.variablesReference = scope["variablesReference"].toInt()
			};
		}
		emit scopesReceived(reqId, scopes);
	} else if (cmd == "variables") {
		QJsonArray jsonVars = response["body"].toObject()["variables"].toArray();
		QVector<Variable> vars(jsonVars.size());
		for (int i = 0; i < jsonVars.size(); i++) {
			QJsonObject var = jsonVars[i].toObject();
			vars[i] = {
				.name = var["name"].toString(),
				.value = var["value"].toString(),
				.type = var["type"].toString()
			};
		}
		emit variablesReceived(reqId, vars);
	} else if (cmd == "setInstructionBreakpoints") {
		qDebug() << response;
		QJsonArray jBreakpoints = response["body"].toObject()["breakpoints"].toArray();
		QVector<uint32_t> breakpoints;
		for (int i = 0; i < jBreakpoints.size(); i++) {
			QJsonObject obj = jBreakpoints[i].toObject();
			if (!obj["verified"].toBool()) {
				qDebug() << "breakpoint not verified";
				continue;
			}
			QString addrStr = obj["instructionReference"].toString();
			breakpoints.push_back(addrStr.toULong(nullptr, 16));
		}
		emit breakpointsReceived(reqId, breakpoints);
	}
}

void DAPClient::error(const QString &message)
{
	qDebug() << "DAP error:" << message;
	emit errorOccurred(message);
}

void DAPClient::handleMessage(const char *msg)
{
	QJsonDocument json = QJsonDocument::fromJson(msg);
	if (!json.isObject()) {
		error("message is not a JSON object");
		return;
	}

	QJsonObject obj = json.object();
	QString t = obj["type"].toString();
	if (t == "event")
		handleEvent(obj);
	else if (t == "response")
		handleResponse(obj);
	else {
		// ?
	}
}

void DAPClient::readInput()
{
	while (true) {
		if (readState == READING_HEADERS) {
			char header[512];
			qint64 nr_read = process->readLine(header, 512);
			// full line is not available
			if (!nr_read)
				return;
			// handle read error
			if (nr_read < 0 || header[nr_read - 1] != '\n') {
				error("read error");
				return;
			}
			// read header
			if (sscanf(header, "Content-Length: %d", &contentLength) == 1) {
				if (contentLength < 1) {
					error("invalid value for Content-Length");
					return;
				}
				content = (char*)malloc(contentLength + 1);
				contentRead = 0;
			} else if ((header[0] == '\r' && header[1] == '\n') || header[0] == '\n') {
				// reached end of headers without Content-Length
				if (!content) {
					error("missing value for Content-Length");
					return;
				}
				readState = READING_CONTENT;
			} else {
				error(QString("unknown header: ") + header);
			}
		} else if (readState == READING_CONTENT) {
			qint64 nr_read = process->read(content + contentRead, contentLength - contentRead);
			if (!nr_read)
				return;
			if (nr_read < 1) {
				error("read error");
				return;
			}
			contentRead += nr_read;
			if (contentRead == contentLength) {
				content[contentLength] = '\0';
				handleMessage(content);
				free(content);
				content = nullptr;
				contentLength = -1;
				contentRead = 0;
				readState = READING_HEADERS;
			}
		}
	}
}

int DAPClient::sendRequest(const QString &command)
{
	QJsonObject req {
		{ "type", "request" },
		{ "command", command }
	};
	return sendJson(req);
}

int DAPClient::sendRequest(const QString &command, QJsonObject &args)
{
	QJsonObject req {
		{ "type", "request" },
		{ "command", command },
		{ "arguments", args }
	};
	return sendJson(req);
}

int DAPClient::sendJson(QJsonObject &obj)
{
	static int seq = 1;
	obj["seq"] = seq;

	QJsonDocument doc(obj);
	QByteArray bytes = doc.toJson();
	int len = bytes.size();

	process->write(QString("Content-Length: %1\r\n\r\n").arg(len).toUtf8());
	process->write(bytes);
	return seq++;
}
