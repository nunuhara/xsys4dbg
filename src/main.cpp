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

#include <QApplication>
#include <QCommandLineParser>
#include <QGuiApplication>

#include "version.hpp"
#include "mainwindow.hpp"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("nunuhara");
	QCoreApplication::setOrganizationDomain("haniwa.technology");
	QCoreApplication::setApplicationName("xsys4dbg");
	QCoreApplication::setApplicationVersion(XSYS4DBG_VERSION);

	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::applicationName());
	parser.addHelpOption();
	parser.addVersionOption();
	parser.process(app);

	MainWindow w;
	w.setWindowTitle("xsys4dbg");
	// TODO positional arguments
	w.show();
	return app.exec();
}


