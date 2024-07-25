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

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>

#include <QDebug>
#include <iostream>

#include "debugger.hpp"

Debugger::Debugger() : QObject()
{
	connect(&client, &DAPClient::outputReceived, this, &Debugger::outputReceived);
	connect(&client, &DAPClient::stackTraceReceived, this, &Debugger::onStackTraceReceived);
	connect(&client, &DAPClient::initialized, this, &Debugger::onInitialized);
	connect(&client, &DAPClient::launched, this, &Debugger::onLaunched);
	connect(&client, &DAPClient::continued, this, &Debugger::onContinued);
	connect(&client, &DAPClient::paused, this, &Debugger::onPaused);
	connect(&client, &DAPClient::terminated, this, &Debugger::onTerminated);
	connect(&client, &DAPClient::terminateFinished, this, &Debugger::initialize);
	connect(&client, &DAPClient::scopesReceived, this, &Debugger::onScopesReceived);
	connect(&client, &DAPClient::variablesReceived, this, &Debugger::onVariablesReceived);
	connect(&client, &DAPClient::breakpointsReceived, this, &Debugger::onBreakpointsReceived);
	connect(&client, &DAPClient::sceneReceived, this, &Debugger::onSceneReceived);
	connect(&client, &DAPClient::renderEntityReceived, this, &Debugger::onRenderEntityReceived);
	connect(&client, &DAPClient::renderPartsReceived, this, &Debugger::onRenderPartsReceived);
	connect(&client, &DAPClient::errorOccurred, [this](const QString &message) {
		emit errorOccurred(QString("DAP Error: ") + message);
	});
}

Debugger::~Debugger()
{
}

bool Debugger::initialize()
{
	if (killing)
		return false;
	QSettings settings;
	QString program = settings.value("xsystem4/path", "xsystem4").toString();
	QStringList arguments;
	arguments << "--debug-api";

	client.initialize(program, arguments, gameDir.absolutePath());
	return true;
}

void Debugger::kill()
{
	killing = true;
	client.kill(3000);
}

bool Debugger::setGameDir(const QString &path)
{
	gameDir = QDir(path);
	if (!gameDir.exists())
		return false;
	if (client.connected())
		client.terminate();
	else
		initialize();
	return true;
}

void Debugger::setInstructionBreakpoint(uint32_t addr)
{
	if (isBreakpoint(addr))
		return;

	instructionBreakpoints.insert(addr);
	client.setInstructionBreakpoints(instructionBreakpoints);
}

void Debugger::clearInstructionBreakpoint(uint32_t addr)
{
	if (!isBreakpoint(addr))
		return;

	instructionBreakpoints.remove(addr);
	client.setInstructionBreakpoints(instructionBreakpoints);
}

void Debugger::toggleInstructionBreakpoint(uint32_t addr)
{
	if (isBreakpoint(addr)) {
		instructionBreakpoints.remove(addr);
	} else {
		instructionBreakpoints.insert(addr);
	}
	client.setInstructionBreakpoints(instructionBreakpoints);
}

bool Debugger::isBreakpoint(uint32_t addr)
{
	return instructionBreakpoints.contains(addr);
}

static QHash<int, renderEntityHandler> renderEntityRequests;

void Debugger::renderEntity(int id, renderEntityHandler handler)
{
	renderEntityRequests[client.requestRenderEntity(id)] = handler;
}

void Debugger::renderParts(int no, renderEntityHandler handler)
{
	renderEntityRequests[client.requestRenderParts(no)] = handler;
}

void Debugger::launch()
{
	client.launch();
}

void Debugger::pause()
{
	client.pause();
}

void Debugger::stop()
{
	client.terminate();
}

void Debugger::next()
{
	client.next();
}

void Debugger::stepIn()
{
	client.stepIn();
}

void Debugger::stepOut()
{
	client.stepOut();
}

// Debugger manages the chain of requests to get full stack trace info
static int pendingStackTrace = 0;
static QHash<int, int> pendingScopes;
static QHash<int, QPair<int, int>> pendingVariables;
static QVector<Debugger::StackFrame> stackTrace;

static int pendingScene = 0;

void Debugger::onStackTraceReceived(int reqId, QVector<DAPClient::StackFrame> &frames)
{
	if (reqId != pendingStackTrace) {
		qDebug() << "unknown stackTrace request:" << reqId;
		return;
	}
	pendingStackTrace = 0;
	stackTrace.resize(frames.size());
	for (int i = 0; i < frames.size(); i++) {
		stackTrace[i].id = frames[i].id;
		stackTrace[i].name = frames[i].name;
		stackTrace[i].address = frames[i].address;
		stackTrace[i].scopes.clear();
	}
	for (int i = 0; i < stackTrace.size(); i++) {
		int reqId = client.requestScopes(frames[i].id);
		pendingScopes[reqId] = i;
	}
}

void Debugger::onScopesReceived(int reqId, QVector<DAPClient::Scope> &scopes)
{
	if (!pendingScopes.contains(reqId)) {
		qDebug() << "unknown scopes request:" << reqId;
		return;
	}

	int frameId = pendingScopes.take(reqId);
	stackTrace[frameId].scopes.resize(scopes.size());
	for (int i = 0; i < scopes.size(); i++) {
		stackTrace[frameId].scopes[i].name = scopes[i].name;
		stackTrace[frameId].scopes[i].presentationHint = scopes[i].presentationHint;
		stackTrace[frameId].scopes[i].variables.clear();
	}
	for (int i = 0; i < scopes.size(); i++) {
		int reqId = client.requestVariables(scopes[i].variablesReference);
		pendingVariables[reqId] = { frameId, i };
	}

	if (pendingVariables.isEmpty() && pendingScopes.isEmpty()) {
		emit stackTraceReceived(stackTrace);
	}
}

void Debugger::onVariablesReceived(int reqId, QVector<DAPClient::Variable> &variables)
{
	if (!pendingVariables.contains(reqId)) {
		qDebug() << "unknown variables request:" << reqId;
		return;
	}
	QPair<int, int> frameScope = pendingVariables.take(reqId);
	int frameId = frameScope.first;
	int scopeNo = frameScope.second;

	Scope &scope = stackTrace[frameId].scopes[scopeNo];
	scope.variables.resize(variables.size());
	for (int i = 0; i < variables.size(); i++) {
		scope.variables[i] = variables[i];
	}

	if (pendingVariables.isEmpty() && pendingScopes.isEmpty()) {
		emit stackTraceReceived(stackTrace);
	}
}

void Debugger::onBreakpointsReceived(int reqId, QVector<uint32_t> &breakpoints)
{
	instructionBreakpoints.clear();
	for (uint32_t bp : breakpoints) {
		instructionBreakpoints.insert(bp);
	}
	emit breakpointsReceived(instructionBreakpoints);
}

void Debugger::onSceneReceived(int reqId, const QVector<SceneEntity> &entities)
{
	if (reqId != pendingScene) {
		qDebug() << "unknown scene request:" << reqId;
		return;
	}
	emit sceneReceived(entities);
}

void Debugger::onRenderEntityReceived(int reqId, int entityId, const QPixmap &pixmap)
{
	if (!renderEntityRequests.contains(reqId)) {
		qDebug() << "unknown renderEntity request:" << reqId;
		return;
	}

	renderEntityHandler cb = renderEntityRequests.take(reqId);
	cb(pixmap);
}

void Debugger::onRenderPartsReceived(int reqId, int partsNo, const QPixmap &pixmap)
{
	if (!renderEntityRequests.contains(reqId)) {
		qDebug() << "unknown renderParts request:" << reqId;
		return;
	}

	renderEntityHandler cb = renderEntityRequests.take(reqId);
	cb(pixmap);
}

void Debugger::onInitialized()
{
	configureOk = true;
	emit initialized();
}

void Debugger::onLaunched()
{
	configureOk = false;
	emit launched();
}

void Debugger::onContinued()
{
	configureOk = false;
	emit continued();
}

void Debugger::onPaused()
{
	configureOk = true;
	emit paused();

	stackTrace.clear();
	pendingVariables.clear();
	pendingScopes.clear();
	pendingStackTrace = client.requestStackTrace();
	pendingScene = client.requestScene();
}

void Debugger::onTerminated()
{
	configureOk = false;
	emit terminated();
}

bool Debugger::canConfigure()
{
	return configureOk;
}
