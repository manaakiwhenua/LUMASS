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
#include <QPainter>
#include <QComboBox>

#include "NMComponentListItemDelegate.h"
#include "NMLayer.h"

#define ctxCompLID "NMComponentListItemDelegate"

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

    const int level = index.internalId() % 100;
    switch (level)
    {
    case 1:
		{
			QString legendTypeStr = index.data(Qt::UserRole).toString();
			if (legendTypeStr == "NM_LEGEND_RAMP" && index.row() == 3)
			{
				painter->save();

				QIcon rampIcon = index.data(Qt::DecorationRole).value<QIcon>();
				QRect rampRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt);

				//ToDo:: find out how to get rid of these hard coded padding values below !!
				//       it might not work with every style
				//QPixmap rampPix = rampIcon.pixmap(QSize(opt.decorationSize.width(), rampRect.height()-4));
				QPixmap rampPix = rampIcon.pixmap(QSize(16, rampRect.height()-4));
				painter->drawPixmap(rampRect.topLeft()+QPoint(0,2), rampPix);

				QRect textRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &opt);
				QString upperStr = index.data(Qt::UserRole+1).toString();
				QString lowerStr = index.data(Qt::UserRole+2).toString();
				painter->setFont(index.data(Qt::FontRole).value<QFont>());
				QFontMetrics fm(painter->font());
				const int lh = fm.height();
				painter->drawText(textRect.x(), textRect.top()+lh, upperStr);
				painter->drawText(textRect.x(), textRect.bottom(), lowerStr);

				painter->restore();
				return;
			}
		}
		break;

    case 2:
		{
			opt.rect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &opt);

			painter->save();
			QString valStr1 = index.data().toString();
			QString valStr2 = index.sibling(index.row(), 1).data().toString();
			QRect valRect1 = QRect(opt.rect.x(),
						opt.rect.y(), opt.rect.width() / 2, opt.rect.height());
			QRect valRect2 = QRect(opt.rect.x() + (opt.rect.width() / 2.0 + 0.5),
						opt.rect.y(), opt.rect.width() / 2, opt.rect.height());
			painter->setFont(index.data(Qt::FontRole).value<QFont>());
			painter->setPen(index.data(Qt::ForegroundRole).value<QColor>());
			painter->drawText(valRect1, opt.displayAlignment, valStr1);

			painter->setFont(index.sibling(index.row(), 1).data(Qt::FontRole).value<QFont>());
			painter->setPen(index.sibling(index.row(), 1).data(Qt::ForegroundRole).value<QColor>());
			painter->drawText(valRect2, opt.displayAlignment, valStr2);

			painter->restore();
		}
		return;
		break;
    }

    // we put the DEFAULT switch at the end in case other switches have some subconditions
    // and might fall back onto this default implementation (e.g. level == 1)
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

QWidget*
NMComponentListItemDelegate::createEditor(QWidget* parent,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");

	QWidget* widget = 0;

	const int level = index.internalId() % 100;
	if (level != 2)
	{
		NMDebugCtx(ctxCompLID, << "done!");
		return widget;
	}


	NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());

	bool bok;
	const int legendtype = l->getLegendType();//index.sibling(2, 0).data().toInt(&bok);
	const int classtype = l->getLegendClassType(); //index.sibling(3, 0).data().toInt(&bok);

	const int row = index.row();
	switch(row)
	{
	case 1:
	case 0: // ------------------ LEGEND VALUE FIELD -----------------------------------------
		{
			if (	l->getTable() != 0
				&&	(  	 legendtype == NMLayer::NM_LEGEND_INDEXED
					 ||  legendtype == NMLayer::NM_LEGEND_RAMP
					 ||  legendtype == NMLayer::NM_LEGEND_CLRTAB
					)
			   )
			{
				QStringList cols;
				if (legendtype == NMLayer::NM_LEGEND_INDEXED && classtype == NMLayer::NM_CLASS_UNIQUE)
				{
					// editing of the description field for UNIQUE VALUE
					// doesn't make sense, so bail out!
					if (row == 1)
					{
						NMDebugAI(<< "cannot edit legend description field when "
								<< "mapping unique values!" << std::endl);
						NMDebugCtx(ctxCompLID, << "done!");
						return widget;
					}
					cols.append(l->getStringColumns());
					cols.append(l->getNumericColumns(true));
				}
				// editing the value field for colour tables doesn't make sense
				// so, bail out!
				else if (legendtype == NMLayer::NM_LEGEND_CLRTAB)
				{
					if (row == 0)
					{
						NMDebugAI(<< "cannot edit the legend value field when "
								<< "mapping table colours!" << std::endl);
						NMDebugCtx(ctxCompLID, << "done!");
						return widget;
					}
					cols.append(l->getStringColumns());
					cols.append(l->getNumericColumns(false));
				}
				else
				{
					cols.append(l->getNumericColumns(false));
				}

				int current = 0;
				QString currentValue = row == 0 ? l->getLegendValueField() : l->getLegendDescrField();
				for (int c=0; c < cols.size(); ++c)
				{
					if (cols.at(c).compare(currentValue, Qt::CaseInsensitive) == 0)
					{
						current = c;
						break;
					}
				}

				QComboBox* box = new QComboBox(parent);
				box->addItems(cols);
				box->setCurrentIndex(current);

				NMDebugCtx(ctxCompLID, << "done!");
				return box;
			}
		}
		break;

	default:
		break;
	}

	NMDebugCtx(ctxCompLID, << "done!");
	return widget;
}

void
NMComponentListItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");

	const int level = index.internalId() % 100;
	if (level != 2)
	{
		NMDebugCtx(ctxCompLID, << "done!");
		return;
	}

	NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());

	const int row = index.row();
	switch(row)
	{
		case 1:
		case 0:
		{
			QComboBox* box = static_cast<QComboBox*>(editor);
			QString valuefield = box->currentText();
			if (row == 0)
			{
				if (valuefield.compare(l->getLegendValueField(), Qt::CaseInsensitive) != 0)
				{
					l->setLegendValueField(valuefield);
					l->updateMapping();
				}
			}
			else if (row == 1)
			{
				if (valuefield.compare(l->getLegendDescrField(), Qt::CaseInsensitive) != 0)
				{
					l->setLegendDescrField(valuefield);
					l->updateLegend();
				}
			}
		}
		break;

	default:
		break;
	}

	NMDebugCtx(ctxCompLID, << "done!");
}

void
NMComponentListItemDelegate::setEditorData(QWidget* editor,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");

	//const int level = index.internalId() % 100;
    //
	//if (level != 2)
	//	return;
    //
	//NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());
    //
	//const int row = index.row();
	//switch(row)
	//{
	//	case 0:
	//	{
	//	}
	//	break;
    //
	//default:
	//	break;
	//}

	NMDebugCtx(ctxCompLID, << "done!");
}

void
NMComponentListItemDelegate::updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");
	QRect geom = option.rect;
	//geom.adjust(option.rect.width()/2.0, 0, 0, 50);
	editor->setGeometry(geom);
	NMDebugCtx(ctxCompLID, << "done!");
}
