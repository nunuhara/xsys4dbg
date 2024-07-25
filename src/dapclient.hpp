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

#ifndef XSYS4DBG_DAP_CLIENT_HPP
#define XSYS4DBG_DAP_CLIENT_HPP

#include <QObject>
#include <QSet>
#include <QVariant>
#include <QVector>
#include <QString>
#include "xsystem4.hpp"

class QStringList;
class QProcess;
class QJsonObject;
class QPixmap;

class DAPClient : public QObject
{
	Q_OBJECT
public:
	DAPClient();
	~DAPClient();

	bool connected();
	void initialize(const QString &program, const QStringList &arguments,
			const QString &workingDirectory);
	void terminate();
	void kill(int msec);
	int requestStackTrace();
	int requestScopes(int frameId);
	int requestVariables(int variablesReference);
	int setInstructionBreakpoints(QSet<uint32_t> locations);
	int requestScene();
	int requestRenderEntity(int entityId);
	int requestSpriteTexture(int spriteId);
	int requestRenderParts(int partsId);

	struct StackFrame {
		int id;
		QString name;
		int address;
	};

	struct Scope {
		QString name;
		QString presentationHint;
		int variablesReference;
	};

	struct Variable {
		QString name;
		QString value;
		QString type;
		int variablesReference;
	};

public slots:
	void launch();
	void pause();
	void next();
	void stepIn();
	void stepOut();

signals:
	void initialized();
	void launched();
	void paused();
	void continued();
	void terminated();
	void terminateFinished();
	void outputReceived(const QString &category, const QString &message);
	void stackTraceReceived(int reqId, QVector<StackFrame> &frames);
	void scopesReceived(int reqId, QVector<Scope> &scopes);
	void variablesReceived(int reqId, QVector<Variable> &variables);
	void breakpointsReceived(int reqId, QVector<uint32_t> &breakpoints);
	void sceneReceived(int reqId, const QVector<SceneEntity> &entities);
	void renderEntityReceived(int reqId, int entityId, const QPixmap &pixmap);
	void renderPartsReceived(int reqId, int partsNo, const QPixmap &pixmap);
	void errorOccurred(const QString &message);

private:

	int sendJson(QJsonObject &obj);
	int sendRequest(const QString &command);
	int sendRequest(const QString &command, QJsonObject &args);
	void readInput();
	void handleMessage(const char *msg);
	void handleResponse(QJsonObject &response);
	void handleEvent(QJsonObject &event);
	void error(const QString &message);

	enum ReadState {
		READING_HEADERS,
		READING_CONTENT
	};

	enum ReadState readState = READING_HEADERS;
	char *content = nullptr;
	int contentLength = -1;
	int contentRead = 0;

	enum DebugState {
		DS_NOT_STARTED,
		DS_INITIALIZING,
		DS_CONFIGURING,
		DS_RUNNING,
		DS_PAUSED
	};

	enum DebugState state = DS_NOT_STARTED;
	QProcess *process = nullptr;
};

#endif
