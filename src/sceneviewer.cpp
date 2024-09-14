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
#include "debugger.hpp"

struct SceneNode
{
	SceneNode() : parent(nullptr), type(ROOT) {};
	explicit SceneNode(SceneEntity *e, SceneNode *parentNode = nullptr);
	explicit SceneNode(Parts *p, SceneNode *parentNode);
	~SceneNode() { qDeleteAll(children); };

	int row() const {
		if (parent)
			return parent->children.indexOf(const_cast<SceneNode*>(this));
		return 0;
	}

	SceneNode *parent;
	QVector<SceneNode*> children;

	enum SceneNodeType {
		ROOT,
		ENTITY,
		PARTS,
	} type;
	union {
		SceneEntity *entity;
		Parts *parts;
	} data;

	QPixmap image;
};

SceneNode::SceneNode(SceneEntity *e, SceneNode *parentNode)
	: parent(parentNode)
	, type(ENTITY)
{
	data.entity = e;
	for (Parts &p : e->parts) {
		children.append(new SceneNode(&p, this));
	}
}

SceneNode::SceneNode(Parts *p, SceneNode *parentNode)
	: parent(parentNode)
	, type(PARTS)
{
	data.parts = p;
	for (Parts &child : p->children) {
		children.append(new SceneNode(&child, this));
	}
}

SceneViewer::SceneViewer(QWidget *parent)
	: QSplitter(parent)
{
	imageArea = new QScrollArea;
	imageArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	listView = new QTreeView;
	detailView = new QTreeView;

	QSplitter *rightPane = new QSplitter(Qt::Vertical);
	rightPane->addWidget(listView);
	rightPane->addWidget(detailView);

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
	delete detailView->model();
}

void SceneViewer::onSceneReceived(const QVector<SceneEntity> &sceneEntities)
{
	sceneId++;
	entities = sceneEntities;
	entityImages.clear();
	entityImages.resize(sceneEntities.size());

	QAbstractItemModel *oldModel = listView->model();
	listView->setModel(new SceneTreeModel(entities));
	delete oldModel;

	connect(listView, &QTreeView::activated, this, &SceneViewer::onActivated);
	connect(listView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &SceneViewer::onCurrentChanged);
}

static SceneNode *getNode(const QModelIndex &index)
{
	if (!index.isValid())
		return nullptr;
	SceneNode *node = static_cast<SceneNode*>(index.internalPointer());
	if (node->type == SceneNode::ROOT)
		return nullptr;
	return node;
}

void SceneViewer::onCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
	SceneNode *node = getNode(current);
	if (!node)
		return;

	if (node->type == SceneNode::ENTITY) {
		QAbstractItemModel *oldModel = detailView->model();
		detailView->setModel(new EntityModel(*node->data.entity));
		//tableView->setModel(new SceneEntityModel(*node->data.entity));
		delete oldModel;
	} else if (node->type == SceneNode::PARTS) {
		QAbstractItemModel *oldModel = detailView->model();
		detailView->setModel(new EntityModel(*node->data.parts));
		delete oldModel;
	}
}

void SceneViewer::onActivated(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	int id = sceneId;

	SceneNode *node = static_cast<SceneNode*>(index.internalPointer());
	if (!node->image.isNull()) {
		QLabel *imageLabel = new QLabel;
		imageLabel->setPixmap(node->image);
		imageArea->setWidget(imageLabel);
		return;
	}

	if (node->type == SceneNode::ENTITY) {
		Debugger::getInstance().renderEntity(node->data.entity->id,
				[this, node, id](const QPixmap &pixmap) {
			if (id != sceneId)
				return;
			node->image = pixmap;
			QLabel *imageLabel = new QLabel;
			imageLabel->setPixmap(pixmap);
			imageArea->setWidget(imageLabel);
		});
	} else if (node->type == SceneNode::PARTS) {
		Debugger::getInstance().renderParts(node->data.parts->no,
				[this, node, id](const QPixmap &pixmap) {
			if (id != sceneId)
				return;
			node->image = pixmap;
			QLabel *imageLabel = new QLabel;
			imageLabel->setPixmap(pixmap);
			imageArea->setWidget(imageLabel);
		});
	}
}

SceneTreeModel::SceneTreeModel(const QVector<SceneEntity> &entityList, QObject *parent)
	: QAbstractItemModel(parent)
	, entities(entityList)
{
	rootNode = new SceneNode;
	for (SceneEntity &e : entities) {
		rootNode->children.append(new SceneNode(&e, rootNode));
	}
}

SceneTreeModel::~SceneTreeModel()
{
	delete rootNode;
}

QVariant SceneTreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	SceneNode *node = static_cast<SceneNode*>(index.internalPointer());
	if (node->type == SceneNode::ENTITY) {
		return node->data.entity->name;
	} else if (node->type == SceneNode::PARTS) {
		return QString("parts %1 (%2)")
			.arg(node->data.parts->no)
			.arg(node->data.parts->description());
	}
	return QVariant();
}

Qt::ItemFlags SceneTreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return QAbstractItemModel::flags(index);
}

QVariant SceneTreeModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();
	// TODO?
	return QVariant();
}

QModelIndex SceneTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	SceneNode *parentNode;
	if (!parent.isValid())
		parentNode = rootNode;
	else
		parentNode = static_cast<SceneNode*>(parent.internalPointer());

	if (row < 0 || row >= parentNode->children.count())
		return QModelIndex();
	return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex SceneTreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	SceneNode *childNode = static_cast<SceneNode*>(index.internalPointer());
	SceneNode *parentNode = childNode->parent;

	if (parentNode == rootNode)
		return QModelIndex();

	return createIndex(parentNode->row(), 0, parentNode);
}

int SceneTreeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	SceneNode *parentNode;
	if (!parent.isValid())
		parentNode = rootNode;
	else
		parentNode = static_cast<SceneNode*>(parent.internalPointer());

	return parentNode->children.count();
}

int SceneTreeModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

struct EntityNode
{
	explicit EntityNode(const SceneEntity &e);
	explicit EntityNode(const Parts &p);
	explicit EntityNode(QString name, const PartsState &s, EntityNode *parentNode);
	explicit EntityNode(QString name, const PartsTextLine &line, EntityNode *parentNode);
	explicit EntityNode(QString name, const TextStyle &ts, EntityNode *parentNode);
	explicit EntityNode(QString name, const PartsCpOp &op, EntityNode *parentNode);
	explicit EntityNode(QString name, const PartsParams &p, EntityNode *parentNode);
	explicit EntityNode(QString name, const PartsMotion &m, EntityNode *parentNode);
	explicit EntityNode(QString name, QVariant value, EntityNode *parentNode);
	~EntityNode() { qDeleteAll(children); }

	int row() const {
		if (parent)
			return parent->children.indexOf(const_cast<EntityNode*>(this));
		return 0;
	}

	void loadSpriteParams(const struct Sprite &sp);
	void loadPartsParams(const Parts &p);

	EntityNode *parent;
	QVector<EntityNode*> children;

	QString name;
	QVariant value;
};

void EntityNode::loadSpriteParams(const struct Sprite &sp)
{
	children.append(new EntityNode("Color", sp.color.toString(), this));
	children.append(new EntityNode("Multiply Color", sp.multiply_color.toString(), this));
	children.append(new EntityNode("Add Color", sp.add_color.toString(), this));
	children.append(new EntityNode("Blend Rate", sp.blend_rate, this));
	children.append(new EntityNode("Bounding Rect", sp.rect.toString(), this));
	children.append(new EntityNode("CG No", sp.cg_no, this));
}

void EntityNode::loadPartsParams(const Parts &p)
{
	children.append(new EntityNode("State", p.state, this));
	children.append(new EntityNode("Default", p.deflt, this));
	children.append(new EntityNode("Hovered", p.hovered, this));
	children.append(new EntityNode("Clicked", p.clicked, this));
	children.append(new EntityNode("Local", p.local, this));
	children.append(new EntityNode("Global", p.global, this));
	children.append(new EntityNode("Delegate Index", p.delegateIndex, this));
	children.append(new EntityNode("Sprite Deform", p.spriteDeform, this));
	children.append(new EntityNode("Clickable", p.clickable, this));
	children.append(new EntityNode("OnCursor Sound", p.onCursorSound, this));
	children.append(new EntityNode("OnClick Sound", p.onClickSound, this));
	children.append(new EntityNode("Origin Mode", p.originMode, this));
	children.append(new EntityNode("Linked To", p.linkedTo, this));
	children.append(new EntityNode("Linked From", p.linkedFrom, this));
	children.append(new EntityNode("Draw Filter", p.drawFilter, this));
	children.append(new EntityNode("Message Window", p.messageWindow, this));

	int i = 0;
	EntityNode *motions;
	children.append(motions = new EntityNode("Motions", QVariant(), this));
	for (const PartsMotion &m : p.motions) {
		motions->children.append(new EntityNode(QString("[%1]").arg(i++), m, this));
	}
}

EntityNode::EntityNode(const SceneEntity &e)
	: parent(nullptr)
{
	children.append(new EntityNode("Z", e.z, this));

	if (e.sprite.has_value()) {
		loadSpriteParams(*e.sprite);
	} else if (e.part.has_value()) {
		loadPartsParams(*e.part);
	}
}

EntityNode::EntityNode(const Parts &p)
{
	loadPartsParams(p);
}

EntityNode::EntityNode(QString name, const PartsState &s, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	if (s.type != PARTS_UNINITIALIZED) {
		children.append(new EntityNode("Size", s.size.toString(), this));
		children.append(new EntityNode("Origin Offset", s.originOffset.toString(), this));
		children.append(new EntityNode("Hitbox", s.hitbox.toString(), this));
		children.append(new EntityNode("Surface Area", s.surfaceArea.toString(), this));
	}

	int i = 0;
	EntityNode *child;
	switch (s.type) {
	case PARTS_CG:
		value = "CG";
		children.append(new EntityNode("No", s.data.cg.no, this));
		break;
	case PARTS_TEXT:
		value = "Text";
		children.append(child = new EntityNode("Lines", QVariant(), this));
		for (const PartsTextLine &line : s.data.text.lines) {
			child->children.append(new EntityNode(QString("[%1]").arg(i++), line, this));
		}
		children.append(new EntityNode("Line Space", s.data.text.lineSpace, this));
		children.append(new EntityNode("Cursor", s.data.text.cursor.toString(), this));
		children.append(new EntityNode("Style", s.data.text.textStyle, this));
		break;
	case PARTS_ANIMATION:
		value = "Animation";
		children.append(new EntityNode("Start No", s.data.anim.startNo, this));
		children.append(new EntityNode("Frame Time", s.data.anim.frameTime, this));
		children.append(new EntityNode("Elapsed", s.data.anim.elapsed, this));
		children.append(new EntityNode("Current Frame", s.data.anim.currentFrame, this));
		break;
	case PARTS_NUMERAL:
		value = "Numeral";
		if (s.data.num.haveNum)
			children.append(new EntityNode("Number", s.data.num.num, this));
		children.append(new EntityNode("Space", s.data.num.space, this));
		children.append(new EntityNode("Show Comma", s.data.num.showComma, this));
		children.append(new EntityNode("Length", s.data.num.length, this));
		children.append(new EntityNode("CG No", s.data.num.cgNo, this));
		break;
	case PARTS_HGAUGE:
		value = "HGauge";
		break;
	case PARTS_VGAUGE:
		value = "VGauge";
		break;
	case PARTS_CONSTRUCTION_PROCESS:
		value = "Construction Process";
		for (const PartsCpOp &op : s.data.cproc.operations) {
			children.append(new EntityNode(QString("[%1]").arg(i++), op, this));
		}
		break;
	case PARTS_FLASH:
		value = "Flash";
		children.append(new EntityNode("Filename", s.data.flash.filename, this));
		children.append(new EntityNode("Frame Count", s.data.flash.frame_count, this));
		children.append(new EntityNode("Current Frame", s.data.flash.current_frame, this));
		break;
	case PARTS_UNINITIALIZED:
		value = "<uninitialized>";
		break;
	case PARTS_INVALID:
		value = "<invalid>";
		break;
	}
}

EntityNode::EntityNode(QString name, const TextStyle &ts, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	children.append(new EntityNode("Face", ts.face, this));
	children.append(new EntityNode("Size", ts.size, this));
	children.append(new EntityNode("Bold Width", ts.bold_width, this));
	children.append(new EntityNode("Weight", ts.weight, this));
	children.append(new EntityNode("Edge Top", ts.edge_up, this));
	children.append(new EntityNode("Edge Bottom", ts.edge_down, this));
	children.append(new EntityNode("Edge Left", ts.edge_left, this));
	children.append(new EntityNode("Edge Right", ts.edge_right, this));
	children.append(new EntityNode("Color", ts.color.toString(), this));
	children.append(new EntityNode("Edge Color", ts.edge_color.toString(), this));
	children.append(new EntityNode("Scale X", ts.scale_x, this));
	children.append(new EntityNode("Space Scale X", ts.space_scale_x, this));
	children.append(new EntityNode("Font Spacing", ts.font_spacing, this));
}

EntityNode::EntityNode(QString name, const PartsTextLine &line, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	children.append(new EntityNode("Contents", line.contents, this));
	children.append(new EntityNode("Width", line.width, this));
	children.append(new EntityNode("Height", line.height, this));
}

EntityNode::EntityNode(QString name, const PartsCpOp &op, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	switch (op.type) {
	case PARTS_CP_CREATE: value = "Create"; break;
	case PARTS_CP_CREATE_PIXEL_ONLY: value = "Create (Pixel Only)"; break;
	case PARTS_CP_CG: value = "CG"; break;
	case PARTS_CP_FILL: value = "Fill"; break;
	case PARTS_CP_FILL_ALPHA_COLOR: value = "Fill Alpha Color"; break;
	case PARTS_CP_FILL_AMAP: value = "Fill Alpha Map"; break;
	case PARTS_CP_DRAW_CUT_CG: value = "Draw Cut CG"; break;
	case PARTS_CP_COPY_CUT_CG: value = "Copy Cut CG"; break;
	case PARTS_CP_DRAW_TEXT: value = "Draw Text"; break;
	case PARTS_CP_COPY_TEXT: value = "Copy Text"; break;
	case PARTS_CP_INVALID: value = "<invalid>"; break;
	}
	switch (op.type) {
	case PARTS_CP_CREATE:
	case PARTS_CP_CREATE_PIXEL_ONLY:
		children.append(new EntityNode("Width", op.data.create.width, this));
		children.append(new EntityNode("Height", op.data.create.height, this));
		break;
	case PARTS_CP_CG:
		children.append(new EntityNode("No", op.data.cg.no, this));
		break;
	case PARTS_CP_FILL:
	case PARTS_CP_FILL_ALPHA_COLOR:
	case PARTS_CP_FILL_AMAP:
		children.append(new EntityNode("Rectangle", op.data.fill.rect.toString(), this));
		children.append(new EntityNode("Color", op.data.fill.color.toString(), this));
		break;
	case PARTS_CP_DRAW_CUT_CG:
	case PARTS_CP_COPY_CUT_CG:
		children.append(new EntityNode("CG No", op.data.cutCg.cgNo, this));
		children.append(new EntityNode("Destination", op.data.cutCg.dst.toString(), this));
		children.append(new EntityNode("Source", op.data.cutCg.src.toString(), this));
		children.append(new EntityNode("Interpolation Type", op.data.cutCg.interpType, this));
		break;
	case PARTS_CP_DRAW_TEXT:
	case PARTS_CP_COPY_TEXT:
		children.append(new EntityNode("Text", op.data.text.text, this));
		children.append(new EntityNode("Position", op.data.text.pos.toString(), this));
		children.append(new EntityNode("Line Space", op.data.text.lineSpace, this));
		children.append(new EntityNode("Style", op.data.text.style, this));
		break;
	case PARTS_CP_INVALID:
		break;
	}
}

EntityNode::EntityNode(QString name, const PartsParams &p, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	children.append(new EntityNode("Z", p.z, this));
	children.append(new EntityNode("Position", p.pos.toString(), this));
	children.append(new EntityNode("Show", p.show, this));
	children.append(new EntityNode("Alpha", p.alpha, this));
	children.append(new EntityNode("Scale", p.scale.toString(), this));
	children.append(new EntityNode("Rotation", p.rotation.toString(), this));
	children.append(new EntityNode("Add Color", p.addColor.toString(), this));
	children.append(new EntityNode("Multiply Color", p.mulColor.toString(), this));
}

EntityNode::EntityNode(QString name, const PartsMotion &m, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
{
	switch (m.type) {
	case PARTS_MOTION_POS: value = "Position"; break;
	case PARTS_MOTION_VIBRATION_SIZE: value = "Vibration Size"; break;
	case PARTS_MOTION_ALPHA: value = "Alpha"; break;
	case PARTS_MOTION_CG: value = "CG"; break;
	case PARTS_MOTION_NUMERAL_NUMBER: value = "Numeral Number"; break;
	case PARTS_MOTION_HGAUGE_RATE: value = "HGauge Rate"; break;
	case PARTS_MOTION_VGAUGE_RATE: value = "VGauge Rate"; break;
	case PARTS_MOTION_MAG_X: value = "X-Magnitude"; break;
	case PARTS_MOTION_MAG_Y: value = "Y-Magnitude"; break;
	case PARTS_MOTION_ROTATE_X: value = "X-Rotation"; break;
	case PARTS_MOTION_ROTATE_Y: value = "Y-Rotation"; break;
	case PARTS_MOTION_ROTATE_Z: value = "Z-Rotation"; break;
	case PARTS_MOTION_INVALID: value = "<invalid>"; return;
	}
	switch (m.type) {
	case PARTS_MOTION_POS:
	case PARTS_MOTION_VIBRATION_SIZE:
		children.append(new EntityNode("Begin", m.begin.pos.toString(), this));
		children.append(new EntityNode("End", m.end.pos.toString(), this));
		break;
	case PARTS_MOTION_ALPHA:
	case PARTS_MOTION_CG:
	case PARTS_MOTION_NUMERAL_NUMBER:
		children.append(new EntityNode("Begin", m.begin.i, this));
		children.append(new EntityNode("End", m.end.i, this));
		break;
	case PARTS_MOTION_HGAUGE_RATE:
	case PARTS_MOTION_VGAUGE_RATE:
	case PARTS_MOTION_MAG_X:
	case PARTS_MOTION_MAG_Y:
	case PARTS_MOTION_ROTATE_X:
	case PARTS_MOTION_ROTATE_Y:
	case PARTS_MOTION_ROTATE_Z:
		children.append(new EntityNode("Begin", m.begin.f, this));
		children.append(new EntityNode("End", m.end.f, this));
		break;
	case PARTS_MOTION_INVALID:
		break;
	}
	children.append(new EntityNode("Begin Time", m.beginTime, this));
	children.append(new EntityNode("End Time", m.endTime, this));
}

EntityNode::EntityNode(QString name, QVariant value, EntityNode *parentNode)
	: parent(parentNode)
	, name(name)
	, value(value)
{
}

EntityModel::EntityModel(const SceneEntity &e, QObject *parent)
	: QAbstractItemModel(parent)
{
	rootNode = new EntityNode(e);
}

EntityModel::EntityModel(const Parts &p, QObject *parent)
	: QAbstractItemModel(parent)
{
	rootNode = new EntityNode(p);
}

EntityModel::~EntityModel()
{
	delete rootNode;
}

QVariant EntityModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	EntityNode *node = static_cast<EntityNode*>(index.internalPointer());
	switch (index.column()) {
	case 0: return node->name;
	case 1: return node->value;
	}
	return QVariant();
}

Qt::ItemFlags EntityModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;
	return QAbstractItemModel::flags(index);
}

QVariant EntityModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();
	// TODO?
	return QVariant();
}

QModelIndex EntityModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	EntityNode *parentNode;
	if (!parent.isValid())
		parentNode = rootNode;
	else
		parentNode = static_cast<EntityNode*>(parent.internalPointer());

	if (row < 0 || row >= parentNode->children.count())
		return QModelIndex();
	return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex EntityModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	EntityNode *childNode = static_cast<EntityNode*>(index.internalPointer());
	EntityNode *parentNode = childNode->parent;

	if (parentNode == rootNode)
		return QModelIndex();

	return createIndex(parentNode->row(), 0, parentNode);
}

int EntityModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0)
		return 0;

	EntityNode *parentNode;
	if (!parent.isValid())
		parentNode = rootNode;
	else
		parentNode = static_cast<EntityNode*>(parent.internalPointer());

	return parentNode->children.count();
}

int EntityModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}
