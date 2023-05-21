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
#include "codeviewer.hpp"
#include "debugger.hpp"
#include "mainwindow.hpp"
#include "outputlog.hpp"
#include "settingsdialog.hpp"
#include "version.hpp"

extern "C" {
#include <string.h>
#include "system4/ain.h"
#include "system4/ini.h"
#include "system4/string.h"
#include "system4/utfsjis.h"
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	createLandingActions();
	createStatusBar();

	readSettings();
	setUnifiedTitleAndToolBarOnMac(true);

	connect(&Debugger::getInstance(), &Debugger::errorOccurred, this, &MainWindow::error);
}

MainWindow::~MainWindow()
{
	recentMenu->clear();
	for (QAction *act : recentActions) {
		delete act;
	}
	if (ain)
		ain_free(ain);
	if (viewMenu)
		delete viewMenu;
}

void MainWindow::open()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Open Game Directory"));
	if (!dirName.isEmpty()) {
		openGameDir(dirName);
	}
}

void MainWindow::about()
{
	QMessageBox::about(this, tr("About xsys4dbg"),
			"xsys4dbg version " XSYS4DBG_VERSION);
}

void MainWindow::createLandingActions()
{
	const QIcon openIcon = QIcon::fromTheme("document-open");
	openAct = new QAction(openIcon, tr("&Open..."), this);
	openAct->setShortcuts(QKeySequence::Open);
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, &QAction::triggered, this, &MainWindow::open);

	const QIcon exitIcon = QIcon::fromTheme("application-exit");
	exitAct = new QAction(exitIcon, tr("E&xit"), this);
	exitAct->setShortcuts(QKeySequence::Quit);
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, &QAction::triggered, this, &QWidget::close);

	settingsAct = new QAction(tr("Settings"), this);
	settingsAct->setStatusTip(tr("Change settings"));
	connect(settingsAct, &QAction::triggered, [this]{
		SettingsDialog dialog;
		dialog.exec();
	});

	const QIcon aboutIcon = QIcon::fromTheme("help-about");
	aboutAct = new QAction(aboutIcon, tr("&About"), this);
	aboutAct->setStatusTip(tr("About xsys4dbg"));
	connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAct);
	recentMenu = fileMenu->addMenu(openIcon, tr("Open &Recent"));
	fileMenu->addAction(exitAct);

	debugMenu = menuBar()->addMenu(tr("&Debug"));
	debugMenu->addAction(settingsAct);

	helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);

	updateRecentActions();
}

void MainWindow::updateRecentActions()
{
	// clear old actions
	recentMenu->clear();
	for (QAction *act : recentActions) {
		delete act;
	}
	recentActions.clear();

	QSettings settings;
	QStringList recent = settings.value("recent").toStringList();
	for (int i = 0; i < recent.size(); i++) {
		QString name = recent[i];
		QAction *act = new QAction(name);
		connect(act, &QAction::triggered, this, [this, name] { openGameDir(name); });
		recentMenu->addAction(act);
		recentActions.append(act);
	}
}

void MainWindow::createOpenedActions()
{
	Debugger *dbg = &Debugger::getInstance();

	const QPixmap runImage = QPixmap(":/icons/debug-start.svg");
	const QIcon runIcon = QIcon(runImage);
	runAct = new QAction(runIcon, tr("&Run"), this);
	runAct->setStatusTip(tr("Begin execution"));
	runAct->setEnabled(false);
	connect(runAct, &QAction::triggered, dbg, &Debugger::launch);

	const QPixmap pauseImage = QPixmap(":/icons/debug-pause.svg");
	const QIcon pauseIcon = QIcon(pauseImage);
	pauseAct = new QAction(pauseIcon, tr("&Pause"), this);
	pauseAct->setStatusTip(tr("Pause execution"));
	pauseAct->setEnabled(false);
	connect(pauseAct, &QAction::triggered, dbg, &Debugger::pause);

	const QPixmap stopImage = QPixmap(":/icons/debug-stop.svg");
	const QIcon stopIcon = QIcon(stopImage);
	stopAct = new QAction(stopIcon, tr("&Stop"), this);
	stopAct->setStatusTip(tr("Halt execution"));
	stopAct->setEnabled(false);
	connect(stopAct, &QAction::triggered, dbg, &Debugger::stop);

	const QPixmap nextImage = QPixmap(":/icons/debug-step-over.svg");
	const QIcon nextIcon = QIcon(nextImage);
	nextAct = new QAction(nextIcon, tr("&Next"), this);
	nextAct->setStatusTip(tr("Execute next instruction (in current function)"));
	nextAct->setEnabled(false);
	connect(nextAct, &QAction::triggered, dbg, &Debugger::next);

	const QPixmap stepImage = QPixmap(":/icons/debug-step-into.svg");
	const QIcon stepIcon = QIcon(stepImage);
	stepAct = new QAction(stepIcon, tr("Step &In"), this);
	stepAct->setStatusTip(tr("Execute next instruction"));
	stepAct->setEnabled(false);
	connect(stepAct, &QAction::triggered, dbg, &Debugger::stepIn);

	const QPixmap finishImage = QPixmap(":/icons/debug-step-out.svg");
	const QIcon finishIcon = QIcon(finishImage);
	finishAct = new QAction(finishIcon, tr("Step &Out"), this);
	finishAct->setStatusTip(tr("Execute next instruction"));
	finishAct->setEnabled(false);
	connect(finishAct, &QAction::triggered, dbg, &Debugger::stepOut);

	// menus
	viewMenu = new QMenu(tr("&View"));
	menuBar()->insertMenu(debugMenu->menuAction(), viewMenu);
	debugMenu->clear();
	debugMenu->addAction(runAct);
	debugMenu->addAction(pauseAct);
	debugMenu->addAction(stopAct);
	debugMenu->addAction(nextAct);
	debugMenu->addAction(stepAct);
	debugMenu->addAction(finishAct);
	debugMenu->addAction(settingsAct);

	// toolbar
	QToolBar *debugToolBar = addToolBar(tr("Debug"));
	debugToolBar->addAction(runAct);
	debugToolBar->addAction(pauseAct);
	debugToolBar->addAction(stopAct);
	debugToolBar->addAction(nextAct);
	debugToolBar->addAction(stepAct);
	debugToolBar->addAction(finishAct);

	functionSelector = new QComboBox;
	functionSelector->setMinimumSize(400, 0);
	functionSelector->setEditable(true);
	connect(functionSelector, QOverload<int>::of(&QComboBox::activated), this, [this] {
		codeViewer->setFunction(functionSelector->currentText());
	});
	debugToolBar->addWidget(functionSelector);

	connect(dbg, &Debugger::initialized, [this]{
		runAct->setEnabled(true);
		pauseAct->setDisabled(true);
		stopAct->setDisabled(true);
		nextAct->setDisabled(true);
		stepAct->setDisabled(true);
		finishAct->setDisabled(true);
	});
	connect(dbg, &Debugger::launched, [this]{
		runAct->setDisabled(true);
		pauseAct->setEnabled(true);
		stopAct->setEnabled(true);
		nextAct->setDisabled(true);
		stepAct->setDisabled(true);
		finishAct->setDisabled(true);
	});
	connect(dbg, &Debugger::paused, [this]{
		runAct->setEnabled(true);
		pauseAct->setDisabled(true);
		stopAct->setEnabled(true);
		nextAct->setEnabled(true);
		stepAct->setEnabled(true);
		finishAct->setEnabled(true);
	});
	connect(dbg, &Debugger::continued, [this]{
		runAct->setDisabled(true);
		pauseAct->setEnabled(true);
		stopAct->setEnabled(true);
		nextAct->setDisabled(true);
		stepAct->setDisabled(true);
		finishAct->setDisabled(true);
	});
	connect(dbg, &Debugger::terminated, [this]{
		runAct->setDisabled(true);
		pauseAct->setDisabled(true);
		stopAct->setDisabled(true);
		nextAct->setDisabled(true);
		stepAct->setDisabled(true);
		finishAct->setDisabled(true);
	});
}

void MainWindow::createStatusBar()
{
	status(tr("Ready"));
}

void MainWindow::createDockWindows()
{
	outputLog = new OutputLog(this);
	addDockWidget(Qt::BottomDockWidgetArea, outputLog);
	resizeDocks({outputLog}, {100}, Qt::Vertical);
	viewMenu->addAction(outputLog->toggleViewAction());

	connect(&Debugger::getInstance(), &Debugger::outputReceived,
			outputLog, &OutputLog::outputReceived);
}

void MainWindow::createViewer()
{
	tabWidget = new QTabWidget;
	tabWidget->setMovable(true);
	tabWidget->setTabsClosable(false);

	const QPixmap codeImage = QPixmap(":/icons/file-binary.svg");
	const QIcon codeIcon = QIcon(codeImage);
	codeViewer = new CodeViewer;
	tabWidget->addTab(codeViewer, codeIcon, "Code");
	setCentralWidget(tabWidget);

	connect(codeViewer, &CodeViewer::functionChanged,
			this, &MainWindow::onFunctionChanged);
}

void MainWindow::readSettings()
{
        QSettings settings;
        const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
        if (geometry.isEmpty()) {
                const QRect availableGeometry = screen()->availableGeometry();
                resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
                move((availableGeometry.width() - width()) / 2,
                     (availableGeometry.height() - height()) / 2);
        } else {
                restoreGeometry(geometry);
        }
}

void MainWindow::writeSettings()
{
	QSettings settings;
        settings.setValue("geometry", saveGeometry());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	Debugger::getInstance().kill();
}

void MainWindow::error(const QString &message)
{
        QMessageBox::critical(this, "xsys4dbg", message, QMessageBox::Ok);
}

void MainWindow::status(const QString &message)
{
	statusBar()->showMessage(message);
}

void MainWindow::onFunctionChanged(int fno)
{
	int i = functionSelector->findText(ain->functions[fno].name);
	functionSelector->setCurrentIndex(i);
}

static char *conv_utf8(const char *sjis)
{
	return sjis2utf(sjis, 0);
}

static void ini_free(struct ini_entry *ini, int nr_entries)
{
	for (int i = 0; i < nr_entries; i++) {
		ini_free_entry(&ini[i]);
	}
	free(ini);
}

void MainWindow::openGameDir(const QString &path)
{
	QDir dir(path);
	QFile file(dir.filePath("System40.ini"));
	if (!file.exists()) {
		file.setFileName(dir.filePath("AliceStart.ini"));
		if (!file.exists()) {
			error("Couldn't find .ini file in given directory");
			return;
		}
	}

	int ini_size;
	struct ini_entry *ini = ini_parse((const char*)file.fileName().toUtf8(), &ini_size);
	if (!ini) {
		error("Failed to parse .ini file");
		return;
	}

	const char *code_name = NULL;
	const char *game_name = NULL;
	for (int i = 0; i < ini_size; i++) {
		if (!strcmp(ini[i].name->text, "CodeName")) {
			if (ini[i].value.type != INI_STRING) {
				error(".ini \"CodeName\" value is not a string");
				ini_free(ini, ini_size);
				return;
			}
			code_name = ini[i].value.s->text;
			continue;
		}
		if (!strcmp(ini[i].name->text, "GameName")) {
			if (ini[i].value.type != INI_STRING)
				continue;
			game_name = ini[i].value.s->text;
			continue;
		}
	}
	if (code_name == NULL) {
		error(".ini file has no \"CodeName\" value");
		ini_free(ini, ini_size);
		return;
	}

	QFile ainFile(dir.filePath(code_name));
	if (!ainFile.exists()) {
		error(QString(".ain file \"%1\" does not exist").arg(ainFile.fileName()));
		ini_free(ini, ini_size);
		return;
	}

	status(QString("Loading .ain file: %1").arg(code_name));

	int err;
	struct ain *ainObj;
	if (!(ainObj = ain_open_conv(ainFile.fileName().toUtf8(), conv_utf8, &err))) {
		error(QString("Error opening .ain file: %1").arg(ain_strerror(err)));
		ini_free(ini, ini_size);
		return;
	}

	if (ain)
		ain_free(ain);
	ain = ainObj;

	// initialize debugger UI
	if (!tabWidget) {
		createOpenedActions();
		createViewer();
		createDockWindows();
	}

	for (int i = 0; i < ain->nr_functions; i++) {
		functionSelector->addItem(ain->functions[i].name);
	}

	codeViewer->setAin(ain);

	if (!Debugger::getInstance().setGameDir(path)) {
		error("setGameDir failed");
		ini_free(ini, ini_size);
		return;
	}

	// update window title
	if (game_name) {
		setWindowTitle(QString("xsys4dbg - %1").arg(game_name));
	} else {
		setWindowTitle(QString("xsys4dbg - %1").arg(code_name));
	}

	// update list of recently opened games
	QSettings settings;
	QStringList recent = settings.value("recent").toStringList();
	recent.removeAll(path);
	recent.prepend(path);
	while (recent.size() > 8) {
		recent.removeLast();
	}
	settings.setValue("recent", recent);
	updateRecentActions();
	ini_free(ini, ini_size);
}
