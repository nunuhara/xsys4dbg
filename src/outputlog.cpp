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

#include <QtWidgets>
#include "debugger.hpp"
#include "mainwindow.hpp"
#include "outputlog.hpp"

OutputLog::OutputLog(MainWindow *parent)
	: QDockWidget(tr("Console Output"), parent)
	, window(parent)
{
	textLog = new QPlainTextEdit;
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setFixedPitch(true);
	font.setPointSize(10);
	textLog->setFont(font);
	textLog->setReadOnly(true);
	textLog->setMaximumBlockCount(200);
	setWidget(textLog);
}

OutputLog::~OutputLog()
{
}

void OutputLog::outputReceived(const QString &source, const QString &message)
{
	textLog->appendPlainText(message.trimmed());
	//textLog->appendPlainText(source + ": " + message);
}
