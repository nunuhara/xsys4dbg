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

#ifndef XSYS4DBG_SETTINGS_DIALOG_HPP
#define XSYS4DBG_SETTINGS_DIALOG_HPP

#include <QDialog>

class QDialogButtonBox;
class QLineEdit;
class QTabWidget;

class GeneralTab : public QWidget
{
	Q_OBJECT
public:
	explicit GeneralTab(QWidget *parent = nullptr);
public slots:
	void writeSettings();
private:
	QLineEdit *xsysPathEdit;
};

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	explicit SettingsDialog(QWidget *parent = nullptr);

private:
	QTabWidget *tabWidget;
	GeneralTab *generalTab;
	QDialogButtonBox *buttonBox;
};

#endif
