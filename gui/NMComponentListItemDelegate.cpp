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
 * NMComponentListItemDelegate.cpp
 *
 *  Created on: 18/12/2013
 *      Author: alex
 */

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
*/

#include <QApplication>

#include "NMComponentListItemDelegate.h"

NMComponentListItemDelegate::NMComponentListItemDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

NMComponentListItemDelegate::~NMComponentListItemDelegate()
{
}

void
NMComponentListItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
    if(!index.isValid())
    	return;

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    opt.decorationSize = index.data(Qt::SizeHintRole).toSize();


    QWidget* widget = 0;
    if (QStyleOptionViewItemV3 *v3 = qstyleoption_cast<QStyleOptionViewItemV3 *>(&opt))
        widget = const_cast<QWidget*>(v3->widget);

    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}

QSize
NMComponentListItemDelegate::sizeHint(const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
	QSize ret;
	if (index.isValid())
	{
		ret = QSize(option.rect.width(), index.data(Qt::SizeHintRole).toSize().height());
	}

	return ret;
}

