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

#include "xsystem4.hpp"

class QScrollArea;
class QTableView;
class QTreeView;
class QPixmap;

class SceneViewer : public QSplitter
{
	Q_OBJECT
public:
	SceneViewer(QWidget *parent = nullptr);
	~SceneViewer();
private slots:
	void onSceneReceived(const QVector<SceneEntity> &entities);
	void onCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
	void onActivated(const QModelIndex &index);
private:
	QScrollArea *imageArea;
	QTreeView *listView;
	QTreeView *detailView;
	QVector<SceneEntity> entities;
	QVector<QPixmap> entityImages;
	int sceneId = 0;
};

struct SceneNode;

class SceneTreeModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit SceneTreeModel(const QVector<SceneEntity> &entityList, QObject *parent = nullptr);
	~SceneTreeModel();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
			const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
private:
	QVector<SceneEntity> entities;
	SceneNode *rootNode;
};

struct EntityNode;

class EntityModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit EntityModel(const SceneEntity &e, QObject *parent = nullptr);
	explicit EntityModel(const Parts &p, QObject *parent = nullptr);
	~EntityModel();

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
			int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column,
			const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
private:
	EntityNode *rootNode;
};

#endif
