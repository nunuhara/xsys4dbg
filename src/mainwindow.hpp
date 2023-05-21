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

#ifndef XSYS4DBG_MAINWINDOW_HPP
#define XSYS4DBG_MAINWINDOW_HPP

#include <QVector>
#include <QMainWindow>

class QComboBox;
class QTabWidget;
class CodeViewer;
class OutputLog;

struct ain;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void open();
	void about();
	void error(const QString &message);
	void status(const QString &message);

	void onFunctionChanged(int fno);

private:
	void createLandingActions();
	void updateRecentActions();
	void createOpenedActions();
	void createStatusBar();
	void createDockWindows();
	void createViewer();
	void readSettings();
	void writeSettings();

	void closeEvent(QCloseEvent *event) override;

	void openGameDir(const QString &path);

	QMenu *fileMenu;
	QMenu *recentMenu;
	QMenu *viewMenu = nullptr;
	QMenu *debugMenu;
	QMenu *helpMenu;

	QTabWidget *tabWidget = nullptr;
	QComboBox *functionSelector;
	CodeViewer *codeViewer;
	OutputLog *outputLog;

	QAction *openAct;
	QAction *exitAct;
	QAction *aboutAct;
	QVector<QAction*> recentActions;

	QAction *runAct;
	QAction *pauseAct;
	QAction *stopAct;
	QAction *nextAct;
	QAction *stepAct;
	QAction *finishAct;

	QAction *settingsAct;

	struct ain *ain = NULL;
};

#endif
