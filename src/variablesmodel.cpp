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

#include "variablesmodel.hpp"

VariableItem::VariableItem(const QVector<QVariant> &data, VariableItem *parent)
	: m_itemData(data), m_parentItem(parent)
{
}

VariableItem::~VariableItem()
{
	qDeleteAll(m_childItems);
}

void VariableItem::appendChild(VariableItem *item)
{
	m_childItems.append(item);
}

VariableItem *VariableItem::child(int row)
{
	if (row < 0 || row >= m_childItems.size())
		return nullptr;
	return m_childItems.at(row);
}

int VariableItem::childCount() const
{
	return m_childItems.count();
}

int VariableItem::row() const
{
	if (m_parentItem)
		return m_parentItem->m_childItems.indexOf(const_cast<VariableItem*>(this));
	return 0;
}

int VariableItem::columnCount() const
{
	return m_itemData.count();
}

QVariant VariableItem::data(int column) const
{
	if (column < 0 || column >= m_itemData.size())
		return QVariant();
	return m_itemData.at(column);
}

VariableItem *VariableItem::parentItem()
{
	return m_parentItem;
}

VariablesModel::VariablesModel(const QVector<Debugger::Scope> &scopes, QObject *parent)
	: QAbstractItemModel(parent)
{
	rootItem = new VariableItem({tr("name"), tr("value")});
	for (const Debugger::Scope &scope : scopes) {
		QVector<QVariant> nodeData { scope.name, "" };
		VariableItem *node = new VariableItem(nodeData, rootItem);
		rootItem->appendChild(node);
		for (const DAPClient::Variable &var : scope.variables) {
			QVector<QVariant> varData = { var.name, var.value };
			VariableItem *varItem = new VariableItem(varData, node);
			node->appendChild(varItem);
		}
	}
}

VariablesModel::~VariablesModel()
{
	delete rootItem;
}

QModelIndex VariablesModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	VariableItem *parentItem;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<VariableItem*>(parent.internalPointer());

	VariableItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex VariablesModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	VariableItem *childItem = static_cast<VariableItem*>(index.internalPointer());
	VariableItem *parentItem = childItem->parentItem();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int VariablesModel::rowCount(const QModelIndex &parent) const
{
	VariableItem *parentItem;
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<VariableItem*>(parent.internalPointer());

	return parentItem->childCount();
}

int VariablesModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return static_cast<VariableItem*>(parent.internalPointer())->columnCount();
	return rootItem->columnCount();
}

QVariant VariablesModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	if (role != Qt::DisplayRole)
		return QVariant();

	VariableItem *item = static_cast<VariableItem*>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags VariablesModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant VariablesModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}
