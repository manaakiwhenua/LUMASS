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
#include <QLineEdit>
#include <QDebug>

#include "NMComponentListItemDelegate.h"
#include "NMLayer.h"
#include "NMImageLayer.h"

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
			if (legendTypeStr == "NM_LEGEND_RAMP" && index.row() == NM_LEGEND_RAMP_ROW)
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
    if (!index.isValid())
	{
        return ret;
    }

    const int level = index.internalId() % 100;
    const int row = index.row();

    NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());
    if (    l
         && level == 1
         && row == NM_LEGEND_RAMP_ROW
         && l->getLegendType() == NMLayer::NM_LEGEND_RAMP
       )
    {
        ret = QSize(index.data(Qt::SizeHintRole).toSize().width(),
                    index.data(Qt::SizeHintRole).toSize().height());
    }
    else
    {
        ret = QStyledItemDelegate::sizeHint(option, index);
    }

	return ret;
}

QWidget*
NMComponentListItemDelegate::createEditor(QWidget* parent,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
    //NMDebugCtx(ctxCompLID, << "...");

	QWidget* widget = 0;

	const int level = index.internalId() % 100;
	const int row = index.row();

	NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());

	bool bok;
	const int legendtype = l->getLegendType();//index.sibling(2, 0).data().toInt(&bok);
	const int classtype = l->getLegendClassType(); //index.sibling(3, 0).data().toInt(&bok);

    // check whether we've got one of those odd Imagine layers with float or double pixel type
    // and an attribute table
    bool bForcePixelMapping = false;
    NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
    if (    il
        &&  (   il->getITKComponentType() == otb::ImageIOBase::FLOAT
             || il->getITKComponentType() == otb::ImageIOBase::DOUBLE
            )
       )
    {
        bForcePixelMapping = true;
    }

    /////////////////////////////////////////
	//if (level != 2)
	//{
	//	NMDebugCtx(ctxCompLID, << "done!");
	//	return widget;
	//}

	//=============================== COLOUR RAMP ================================================
	if (level == 1 && l->getLegendType() == NMLayer::NM_LEGEND_RAMP && row == 2)
	{
        QLineEdit* lineedit = new QLineEdit(parent);


        int shwidth = index.data(Qt::SizeHintRole).toSize().width();

        QRect textRect = QApplication::style()->subElementRect(QStyle::SE_ItemViewItemText, &option, lineedit);
        QRect upperRect(textRect.x()+shwidth+5, option.rect.top(), option.rect.width()-shwidth-10, textRect.height()/3.0);
        QRect lowerRect(textRect.x()+shwidth+5, option.rect.bottom()-upperRect.height(), option.rect.width()-shwidth-10, textRect.height()/3.0);

        //        qDebug() << "mouse pos:   " << mLastMousePos;
        //        qDebug() << "option rect: " << option.rect;
        //        qDebug() << "text rect:   " << textRect;
        //        qDebug() << "upperRect:   " << upperRect;
        //        qDebug() << "lowerRect:   " << lowerRect;


		QString dval;
		QRect editrect;
		bool bhit = false;
		if (upperRect.contains(this->mLastMousePos))
		{
			NMDebugAI(<< "got hit at upper text!" << std::endl);
            //lineedit = new QLineEdit(parent);
			lineedit->setObjectName("le_upper_value");
			dval = QString("%1").arg(l->getUpper());
			editrect = upperRect;
			bhit = true;
		}
		else if (lowerRect.contains(this->mLastMousePos))
		{
			NMDebugAI(<< "got hit at lower text!" << std::endl);
            //lineedit = new QLineEdit(parent);
			lineedit->setObjectName("le_lower_value");
			dval = QString("%1").arg(l->getLower());
			editrect = lowerRect;
			bhit = true;
		}

		if (bhit)
		{
			lineedit->setText(dval);
			lineedit->setGeometry(editrect);
			return lineedit;
		}

	}
    else if (level == 1 && l->getLegendType() == NMLayer::NM_LEGEND_RGB && row > 0)
    {
        QStringList bands;
        for (int b=0; b < il->getTotalNumBands(); ++b)
        {
            bands << QString(tr("Band #%1")).arg(b+1);
        }

        // find the currently selected item
        int curidx = 0;
        for (int p=0; p < bands.size(); ++p)
        {
            if (bands.at(p).compare(il->getLegendName(row)) == 0)
            {
                curidx = p;
                break;
            }
        }

        QComboBox* box = new QComboBox(parent);
        box->addItems(bands);
        box->setCurrentIndex(curidx);

        return box;
    }
	//=============================== LEGEND META DATA ============================================
	else if (level == 2)
	{
		switch(row)
		{
		case 1:
		case 0: // ------------------ LEGEND VALUE FIELD -----------------------------------------
			{
                if (	(   l->getTable() != 0
                         && !bForcePixelMapping
                        )
					&&	(  	 legendtype == NMLayer::NM_LEGEND_INDEXED
						 ||  legendtype == NMLayer::NM_LEGEND_RAMP
						 ||  legendtype == NMLayer::NM_LEGEND_CLRTAB
						)
				   )
				{
					QStringList cols;
                    if (row == 1) // legend descr field
					{
						if (legendtype == NMLayer::NM_LEGEND_CLRTAB)
						{
							cols.append(l->getStringColumns());
							cols.append(l->getNumericColumns(false));
						}
						else
						{
							NMDebugAI(<< "can edit legend description field only when "
									<< "mapping table colours!" << std::endl);

                            //NMDebugCtx(ctxCompLID, << "done!");
							return widget;
						}
					}

					if (legendtype == NMLayer::NM_LEGEND_INDEXED && classtype == NMLayer::NM_CLASS_UNIQUE)
					{
						cols.append(l->getStringColumns());
						cols.append(l->getNumericColumns(true));
					}
					// editing the value field for colour tables doesn't make sense
					// so, bail out!
					else if (legendtype == NMLayer::NM_LEGEND_CLRTAB && row == 0)
					{
						NMDebugAI(<< "cannot edit the legend value field when "
								<< "mapping table colours!" << std::endl);

                        //NMDebugCtx(ctxCompLID, << "done!");
						return widget;
					}
					else
					{
						cols.append(l->getNumericColumns(false));
                        if (il)
                        {
                            cols.prepend(QString("Pixel Values"));
                        }
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

                    //NMDebugCtx(ctxCompLID, << "done!");
					return box;
				}
                else if (   row == 0
                         && (   legendtype == NMLayer::NM_LEGEND_RGB
                             || l->getLegendValueField().startsWith(QString("Band #"))
                            )
                        )
                {
                    QStringList options;
                    options << QString(tr("RGB"));
                    for (int b=0; b < il->getTotalNumBands(); ++b)
                    {
                        options << QString(tr("Band #%1")).arg(b+1);
                    }

                    // find the currently selected item
                    int curidx = 0;
                    for (int p=0; p < options.size(); ++p)
                    {
                        if (options.at(p).compare(il->getLegendValueField()) == 0)
                        {
                            curidx = p;
                            break;
                        }
                    }

                    QComboBox* box = new QComboBox(parent);
                    box->addItems(options);
                    box->setCurrentIndex(curidx);

                    return box;
                }
			}
			break;

        // we don't support classification as yet, so this legend admin item has moved up
        // one row for now ...
        case 3: // was case 4:
			if (legendtype == NMLayer::NM_LEGEND_RAMP)
			{
				QStringList ramps = l->getColourRampStrings();
                // for now remove the manual ramp
                ramps.removeLast();

				int current = 0;
				QString currentRamp = l->getColourRampStr(l->getColourRamp());
				for (int c=0; c < ramps.size(); ++c)
				{
					if (ramps.at(c).compare(currentRamp, Qt::CaseInsensitive) == 0)
					{
						current = c;
						break;
					}
				}
				QComboBox* box = new QComboBox(parent);
				box->addItems(ramps);
				box->setCurrentIndex(current);

                //NMDebugCtx(ctxCompLID, << "done!");
				return box;
			}
			break;

        case 4: // upper // was case 5:
        case 5: // lower // was case 6:
        //case 6:
        //case 7: // this the original nodata row, howeve, nodata is nowhere else used to any effect,
                  // we just display the 'data type's default' no data setting
			{
				if (	(legendtype == NMLayer::NM_LEGEND_INDEXED && classtype != NMLayer::NM_CLASS_UNIQUE)
					||  legendtype == NMLayer::NM_LEGEND_RAMP
				   )
				{
					QLineEdit* lineedit = new QLineEdit(parent);
                    // needs adjustment because of the mess above
                    // note: we have one user row less
                    //int base = Qt::UserRole-4;
                    int base = Qt::UserRole-3;
					QString curVal = index.data(base+index.row()).toString();
					lineedit->setText(curVal);

                    //NMDebugCtx(ctxCompLID, << "done!");
					return lineedit;
				}
			}
			break;


		default:
			break;
		}
	}

    //NMDebugCtx(ctxCompLID, << "done!");
	return widget;
}

void
NMComponentListItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");

	const int level = index.internalId() % 100;
	//if (level != 2)
	//{
	//	NMDebugCtx(ctxCompLID, << "done!");
	//	return;
	//}

	NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());

	const int row = index.row();
	if (level == 1 && l->getLegendType() == NMLayer::NM_LEGEND_RAMP	&& index.row() == 2)
	{
		QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
		bool bok = true;
		double val = lineedit->text().toDouble(&bok);

		if (bok)
		{
			if (lineedit->objectName() == "le_upper_value")
			{
				l->setUpper(val);
			}
			else if (lineedit->objectName() == "le_lower_value")
			{
				l->setLower(val);
			}
			l->updateMapping();
		}
	}
    else if (level == 1 && l->getLegendType() == NMLayer::NM_LEGEND_RGB)
    {
        NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
        if (il == 0)
            return;

        if (row >=1 && row <= 3)
        {
            std::vector<int> bandmap = il->getBandMap();
            if (bandmap.size() != 3)
            {
                bandmap.resize(3, 1);
            }

            QComboBox* box = static_cast<QComboBox*>(editor);
            int band = box->currentIndex() + 1;

            bandmap[row-1] = band;
            il->setBandMap(bandmap);
            l->updateMapping();
        }
    }
	else if (level == 2)
	{
		switch(row)
		{
			case 1:
			case 0:
			{
				QComboBox* box = static_cast<QComboBox*>(editor);
				QString valuefield = box->currentText();
				// -------------------------- LEGEND VALUE FIELD ---------------------------------
				if (row == 0)
				{
					if (valuefield.compare(l->getLegendValueField(), Qt::CaseInsensitive) != 0)
					{

                        // we also adjust the description field; it makes only sense
						// to be different from the value field if we're mapping
						// unique values
						if (l->getLegendClassType() != NMLayer::NM_CLASS_UNIQUE)
							l->setLegendDescrField(valuefield);

                        NMImageLayer* il = qobject_cast<NMImageLayer*>(l);
                        if (valuefield == "RGB")
                        {
                            l->setLegendType(NMLayer::NM_LEGEND_RGB);
                            l->setLegendDescrField("Band Number");
                            std::vector<int> bandmap = il->getBandMap();
                            if (bandmap.size() == 0)
                            {
                                for (int b=0; b < 3; ++b)
                                {
                                    bandmap.push_back(b+1);
                                }
                            }
                            il->setBandMap(bandmap);
                            l->setLegendValueField(valuefield);
                        }
                        else if (valuefield.startsWith(QString("Band #")))
                        {
                            l->setLegendType(NMLayer::NM_LEGEND_RAMP);
                            il->setScalarBand(box->currentIndex());
                            std::vector<int> bandmap;
                            for (int b=0; b < 3; ++b)
                            {
                                bandmap.push_back(box->currentIndex());
                            }
                            il->setBandMap(bandmap);
                            l->setLegendValueField(valuefield);
                        }
                        else
                        {
                            l->setLegendValueField(valuefield);
                        }

                        l->updateMapping();
                        l->updateLegend();
					}
				}
				// --------------------------- LEGEND DESCR FIELD -----------------------------
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

            // this is the legend ramp row
            // since we don't support classfication of values yet, and have consequently taken
            // out the particular row depicting any type of classification, this one moves up
            // one row
            case 3: // was case 4:
				{
					QComboBox* box = static_cast<QComboBox*>(editor);
					QString curVal = box->currentText();
                    // was Qt:UserRole+4 for 8 rows
                    model->setData(index, QVariant(curVal), Qt::UserRole+4);
					//NMLayer::NMColourRamp ramp = l->getColourRampFromStr(curVal);
					//l->setColourRamp(ramp);
					//l->updateMapping();
				}
				break;

            // because of the above, we've just one more row here:
            case 4:

			case 5:
			case 6:
            case 7:
				{
					QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
					QString curVal = lineedit->text();
                    // base was Qt::UserRole-4; when legend classification was still in there
                    int base = Qt::UserRole-3;
					if (!curVal.isEmpty())
					{
						model->setData(index, QVariant(curVal), base+index.row());
					}
				}
				break;

		default:
			break;
		}
	}

	NMDebugCtx(ctxCompLID, << "done!");
}

void
NMComponentListItemDelegate::setEditorData(QWidget* editor,
		const QModelIndex& index) const
{
    // done in createEditor
}

void
NMComponentListItemDelegate::updateEditorGeometry(QWidget* editor,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const
{
	NMDebugCtx(ctxCompLID, << "...");

	const int level = index.internalId() % 100;

    NMLayer* l = static_cast<NMLayer*>(index.data(Qt::UserRole+100).value<void*>());
    QSize shint = index.data(Qt::SizeHintRole).toSize();

    QRect geom;
    if (level == 1 && l->getLegendType() != NMLayer::NM_LEGEND_RGB)
    {
        geom = editor->geometry();
    }
    else if (level == 1 && l->getLegendType() == NMLayer::NM_LEGEND_RGB)
    {
        QRect erect = option.rect;
        erect.setWidth(erect.width()-shint.width());
        erect.setLeft(erect.left()+shint.width()+5);

        geom = erect;
    }
    else if (level == 2)
    {
        geom = option.rect;
    }
	geom.adjust(0, -2, 0, 2);
	editor->setGeometry(geom);

	NMDebugCtx(ctxCompLID, << "done!");
}
