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

#ifndef XSYS4DBG_OUTPUTLOG_HPP
#define XSYS4DBG_OUTPUTLOG_HPP

#include <QDockWidget>

class MainWindow;
class QPlainTextEdit;

class OutputLog : public QDockWidget
{
	Q_OBJECT
public:
	OutputLog(MainWindow *parent = nullptr);
	~OutputLog();

public slots:
	void outputReceived(const QString &source, const QString &message);

private:
	MainWindow *window;
	QPlainTextEdit *textLog;
};

#endif
