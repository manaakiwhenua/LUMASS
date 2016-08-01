 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
#include "NMEditModelComponentDialog.h"
#include <QMetaObject>
#include <QMetaProperty>
#include <QList>
#include <QStringList>
#include <QScopedPointer>
#include <limits>
#include <QVBoxLayout>
#include <QFormLayout>

#include "NMIterableComponent.h"

const std::string NMEditModelComponentDialog::ctx = "NMEditModelComponentDialog";

NMEditModelComponentDialog::NMEditModelComponentDialog(QWidget *parent)
    : QWidget(parent) //,mObj(0), comp(0), proc(0)
{
    //this->setMinimumHeight(300);
    //this->setMinimumWidth(700);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHeightForWidth(false);
    this->setSizePolicy(sizePolicy);
    this->setMouseTracking(true);

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    mScrollArea = new QScrollArea(this);
    mScrollArea->setWidgetResizable(true);
    mScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    mCompEditor = new NMComponentEditor(this, NMComponentEditor::NM_COMPEDITOR_GRPBOX);
    //mCompEditor = new NMComponentEditor(this, NMComponentEditor::NM_COMPEDITOR_TREE);
    mCompEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    mScrollArea->setWidget(mCompEditor);
    //mScrollArea->resize(450, 300);
    mScrollArea->setMinimumWidth(450);
    mScrollArea->setMinimumHeight(250);
    vLayout->addWidget(mScrollArea);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(6);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setFormAlignment(Qt::AlignBottom|Qt::AlignRight|Qt::AlignTrailing);

    QPushButton* btnClose = new QPushButton(this);
    btnClose->setObjectName(QString::fromUtf8("btnClose"));
    btnClose->setText(QString::fromUtf8("Close"));

    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));
    formLayout->setWidget(0, QFormLayout::FieldRole, btnClose);

    vLayout->addLayout(formLayout);

#ifdef BUILD_RASSUPPORT
    mCompEditor->setRasdamanConnectorWrapper(0);
#endif
}

NMEditModelComponentDialog::~NMEditModelComponentDialog()
{
}

void
NMEditModelComponentDialog::setObject(QObject* obj)
{
    mCompEditor->setObject(obj);
    if (mCompEditor->getObject() != 0)
    {
        this->setWindowTitle(obj->objectName());
    }
}

void
NMEditModelComponentDialog::closeEvent(QCloseEvent* event)
{
    emit finishedEditing(mCompEditor->getObject());
	QWidget::closeEvent(event);
}
