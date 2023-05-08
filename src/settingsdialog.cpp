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

#include "settingsdialog.hpp"

SettingsDialog::SettingsDialog(QWidget *parent)
	: QDialog(parent)
{
	generalTab = new GeneralTab;

	tabWidget = new QTabWidget;
	tabWidget->addTab(generalTab, tr("General"));
	//tabWidget->addTab(new FontTab(), tr("Font"));

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(this, &QDialog::accepted, generalTab, &GeneralTab::writeSettings);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(tabWidget);
	layout->addWidget(buttonBox);
	setLayout(layout);

	setWindowTitle(tr("Settings"));
}

GeneralTab::GeneralTab(QWidget *parent)
	: QWidget(parent)
{
	QSettings settings;
	xsysPathEdit = new QLineEdit(settings.value("xsystem4/path", "xsystem4").toString());

	QFormLayout *layout = new QFormLayout;
	layout->addRow(tr("xsystem4 Path:"), xsysPathEdit);
	setLayout(layout);
}

#include <QDebug>
void GeneralTab::writeSettings()
{
	QSettings settings;
	QString xsysPath = xsysPathEdit->text();
	settings.setValue("xsystem4/path", xsysPath.isEmpty() ? "xsystem4" : xsysPath);
}
