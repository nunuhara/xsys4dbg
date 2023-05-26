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

#ifndef XSYS4DBG_SCENEVIEWER_HPP
#define XSYS4DBG_SCENEVIEWER_HPP

#include <QAbstractTableModel>
#include <QSplitter>
#include <QVector>

#include "debugger.hpp"

class QListView;
class QScrollArea;
class QTableView;
class QPixmap;

class SceneViewer : public QSplitter
{
	Q_OBJECT
public:
	SceneViewer(QWidget *parent = nullptr);
	~SceneViewer();
private slots:
	void onSceneReceived(const QVector<DAPClient::SceneEntity> &entities);
	void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
	void onActivated(const QModelIndex &index);
private:
	QScrollArea *imageArea;
	QListView *listView;
	QTableView *tableView;
	QVector<DAPClient::SceneEntity> entities;
	QVector<QPixmap> entityImages;
	int sceneId = 0;
};

class SceneEntityModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	SceneEntityModel(const DAPClient::SceneEntity &e, QObject *parent = nullptr);
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
private:
	DAPClient::SceneEntity entity;
};

#endif
