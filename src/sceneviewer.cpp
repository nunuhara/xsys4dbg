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

#include <QDebug>
#include <QtWidgets>

#include <QAbstractTableModel>

#include "sceneviewer.hpp"

SceneEntityModel::SceneEntityModel(const DAPClient::SceneEntity &e, QObject *parent)
	: QAbstractTableModel(parent)
{
	entity = e;
}

int SceneEntityModel::rowCount(const QModelIndex &parent) const
{
	if (entity.sprite.no < 0)
		return 1;
	return 9;
}

int SceneEntityModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

static QString color_to_string(const DAPClient::Color &c)
{
	return QString("(%1 %2 %3 %4)").arg(c.r).arg(c.g).arg(c.b).arg(c.a);
}

static QString rect_to_string(const DAPClient::Rectangle &r)
{
	return QString("(%1 %2 %3 %4)").arg(r.x).arg(r.y).arg(r.w).arg(r.h);
}

QVariant SceneEntityModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();

	if (index.column() == 0) {
		switch (index.row()) {
		case 0: return "Z";
		case 1: return "ID";
		case 2: return "Color";
		case 3: return "Multiply Color";
		case 4: return "Add Color";
		case 5: return "Blend Rate";
		case 6: return "Draw Method";
		case 7: return "Position";
		case 8: return "CG Number";
		}
	} else if (index.column() == 1) {
		switch (index.row()) {
		case 0: return entity.z;
		case 1: return entity.sprite.no;
		case 2: return color_to_string(entity.sprite.color);
		case 3: return color_to_string(entity.sprite.multiply_color);
		case 4: return color_to_string(entity.sprite.add_color);
		case 5: return entity.sprite.blend_rate;
		case 6: return entity.sprite.draw_method;
		case 7: return rect_to_string(entity.sprite.rect);
		case 8: return entity.sprite.cg_no;
		}
	}
	return QVariant();
}

QVariant SceneEntityModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
		return QVariant();

	switch (section) {
	case 0: return "Property";
	case 1: return "Value";
	}
	return QVariant();
}

SceneViewer::SceneViewer(QWidget *parent)
	: QSplitter(parent)
{
	imageArea = new QScrollArea;
	imageArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	listView = new QListView;
	listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	tableView = new QTableView;
	// XXX: need a dummy model to set initial column width...
	DAPClient::SceneEntity dummy = {
		.name = "<placeholder>",
		.id = -1,
		.z = 0,
		.z2 = 0,
		.sprite = { .no = -1 }
	};
	tableView->setModel(new SceneEntityModel(dummy));
	tableView->setColumnWidth(1, 125);

	QSplitter *rightPane = new QSplitter(Qt::Vertical);
	rightPane->addWidget(listView);
	rightPane->addWidget(tableView);

	addWidget(imageArea);
	addWidget(rightPane);

	// XXX: this gives a good result with the default window size (on my system)...
	//      there doesn't seem to be a good way to control the sizes of a splitter
	//      without subclassing children to override size hints
	setSizes(QList<int>() << 600 << 170);

	connect(&Debugger::getInstance(), &Debugger::sceneReceived,
			this, &SceneViewer::onSceneReceived);
}

SceneViewer::~SceneViewer()
{
	delete listView->model();
	delete tableView->model();
}

void SceneViewer::onSceneReceived(const QVector<DAPClient::SceneEntity> &sceneEntities)
{
	sceneId++;
	entities = sceneEntities;
	entityImages.clear();
	entityImages.resize(sceneEntities.size());

	QStringList names;

	for (DAPClient::SceneEntity &e : entities) {
		names.append(e.name);
	}

	QAbstractItemModel *oldModel = listView->model();
	listView->setModel(new QStringListModel(names));
	delete oldModel;

	connect(listView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &SceneViewer::onCurrentChanged);
	connect(listView, &QAbstractItemView::activated, this, &SceneViewer::onActivated);
}

void SceneViewer::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	int row = current.row();
	if (row < 0 || row >= entities.size()) {
		qDebug() << "invalid entity index:" << row;
		return;
	}

	const DAPClient::SceneEntity &e = entities[row];
	QAbstractItemModel *oldModel = tableView->model();
	tableView->setModel(new SceneEntityModel(e));
	delete oldModel;
}

void SceneViewer::onActivated(const QModelIndex &index)
{
	int row = index.row();
	if (row < 0 || row >= entities.size()) {
		qDebug() << "invalid entity index" << row;
		return;
	}

	if (!entityImages[row].isNull()) {
		QLabel *imageLabel = new QLabel;
		imageLabel->setPixmap(entityImages[row]);
		imageArea->setWidget(imageLabel);
		return;
	}

	int id = sceneId;
	Debugger::getInstance().renderEntity(entities[row].id, [this, row, id](const QPixmap &pixmap) {
		if (id != sceneId)
			return;
		entityImages[row] = pixmap;
		QLabel *imageLabel = new QLabel;
		imageLabel->setPixmap(pixmap);
		imageArea->setWidget(imageLabel);
	});
}
