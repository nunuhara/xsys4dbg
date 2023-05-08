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

#ifndef XSYS4DBG_VARIABLES_MODEL_HPP
#define XSYS4DBG_VARIABLES_MODEL_HPP

#include <QAbstractItemModel>
#include <QVector>
#include "debugger.hpp"

class VariableItem
{
public:
	explicit VariableItem(const QVector<QVariant> &data, VariableItem *parentItem = nullptr);
	~VariableItem();

	void appendChild(VariableItem *child);

	VariableItem *child(int row);
	int childCount() const;
	int columnCount() const;
	QVariant data(int column) const;
	int row() const;
	VariableItem *parentItem();

private:
	QVector<VariableItem*> m_childItems;
	QVector<QVariant> m_itemData;
	VariableItem *m_parentItem;
};

class VariablesModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit VariablesModel(const QVector<Debugger::Scope> &scopes, QObject *parent = nullptr);
	~VariablesModel();

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

	VariableItem *rootItem;
};

#endif
