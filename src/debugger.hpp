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

#ifndef XSYS4DBG_DEBUGGER_HPP
#define XSYS4DBG_DEBUGGER_HPP

#include <functional>
#include <QObject>
#include <QDir>
#include <QSet>
#include <QVector>
#include "dapclient.hpp"

class QProcess;
class QJsonObject;

typedef std::function<void(QVector<DAPClient::Scope>&)> ScopesHandler;
typedef std::function<void(QVector<DAPClient::Variable>&)> VariablesHandler;

class Debugger : public QObject
{
	Q_OBJECT
public:
	static Debugger& getInstance()
	{
		static Debugger instance;
		return instance;
	}
	Debugger(Debugger const&) = delete;
	void operator=(Debugger const&) = delete;

	bool setGameDir(const QString &path);
	bool canConfigure();
	void kill();

	void setInstructionBreakpoint(uint32_t addr);
	void clearInstructionBreakpoint(uint32_t addr);
	void toggleInstructionBreakpoint(uint32_t addr);
	bool isBreakpoint(uint32_t addr);

	struct Scope {
		QString name;
		QString presentationHint;
		QVector<DAPClient::Variable> variables;
	};

	struct StackFrame {
		int id;
		QString name;
		int address;
		QVector<Scope> scopes;
	};

public slots:
	void launch();
	void pause();
	void stop();
	void next();
	void stepIn();
	void stepOut();

signals:
	void initialized();
	void launched();
	void paused();
	void continued();
	void terminated();

	void outputReceived(const QString &source, const QString &message);
	void stackTraceReceived(QVector<StackFrame> &frames);
	void breakpointsReceived(QSet<uint32_t> &breakpoints);

	void errorOccurred(const QString &message);

private slots:
	bool initialize();
	void onInitialized();
	void onLaunched();
	void onContinued();
	void onPaused();
	void onTerminated();
	void onStackTraceReceived(int reqId, QVector<DAPClient::StackFrame> &frames);
	void onScopesReceived(int reqId, QVector<DAPClient::Scope> &scopes);
	void onVariablesReceived(int reqId, QVector<DAPClient::Variable> &variables);
	void onBreakpointsReceived(int reqId, QVector<uint32_t> &breakpoints);

private:
	Debugger();
	~Debugger();

	bool killing = false; // FIXME: this sucks

	bool configureOk = false;
	DAPClient client;

	QSet<uint32_t> instructionBreakpoints;

	QDir gameDir;
};

#endif
