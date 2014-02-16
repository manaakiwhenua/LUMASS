/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2010,2011,2012,2013 Landcare Research New Zealand Ltd
 *
 * This file is part of 'LUMASS', which is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*
 * NMComponentListItemDelegate.h
 *
 *  Created on: 18/12/2013
 *      Author: alex
 */

#ifndef NMCOMPONENTLISTITEMDELEGATE_H_
#define NMCOMPONENTLISTITEMDELEGATE_H_

#include <qstyleditemdelegate.h>
#include <QPoint>

class NMComponentListItemDelegate: public QStyledItemDelegate
{
	Q_OBJECT

public:
	NMComponentListItemDelegate(QObject* parent=0);
	virtual ~NMComponentListItemDelegate();

	void paint(QPainter* painter, const QStyleOptionViewItem& option,
			const QModelIndex& index) const;

	QSize sizeHint(const QStyleOptionViewItem& option,
			const QModelIndex& index) const;

	QWidget* createEditor(QWidget* parent,
			const QStyleOptionViewItem& option,
			const QModelIndex& index) const;

	void setEditorData(QWidget* editor, const QModelIndex& index) const;

	void updateEditorGeometry(QWidget* editor,
			const QStyleOptionViewItem& option,
			const QModelIndex& index) const;

	void setModelData(QWidget* editor,
			QAbstractItemModel* model,
			const QModelIndex& index) const;

	void setLastMousePos(const QPoint& pos)
		{mLastMousePos = pos;}

private:
	QPoint mLastMousePos;

};

#endif /* NMCOMPONENTLISTITEMDELEGATE_H_ */
