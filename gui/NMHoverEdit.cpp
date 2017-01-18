/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2016 Landcare Research New Zealand Ltd
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

#include "NMHoverEdit.h"

#ifndef NM_ENABLE_LOGGER
#   define NM_ENABLE_LOGGER
#   include "nmlog.h"
#   undef NM_ENABLE_LOGGER
#else
#   include "nmlog.h"
#endif
#include "NMGlobalHelper.h"

#include <QSyntaxHighlighter>
#include <QListWidget>
#include <QCompleter>
#include <QTextBrowser>
#include <QStringListModel>
#include <QCheckBox>
#include <QLabel>

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "NMComponentEditor.h"
#include "NMModelController.h"
#include "NMGlobalHelper.h"
#include "NMIterableComponent.h"
#include "NMHoverEditTree.h"
#include "NMParamHighlighter.h"
#include "NMParamEdit.h"


NMHoverEdit::NMHoverEdit(QWidget *parent)
    : QDialog(parent), mComp(0), mProc(0), mPropLevel(0)
{
    this->setModal(false);

    // ==================================================
    //      LAYOUT VISIBLE CONTROLS
    // ==================================================
    QFormLayout* fl = new QFormLayout(this);

    QSizePolicy tepol(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tepol.setVerticalStretch(1);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->setSizePolicy(tepol);

    mTreeWidget = new NMHoverEditTree(this);
    mTreeWidget->setObjectName("treeWidget");
    mTreeWidget->setDragEnabled(true);
    mTreeWidget->setTreeModel(QVariant());
    //splitter->addWidget(mTreeWidget);

    mEdit = new NMParamEdit(this);
    mEdit->setObjectName("ParamEditor");
    mEdit->setSizePolicy(tepol);
    mEdit->setPlaceholderText("Select a parameter");
    mHighlighter = new NMParamHighlighter(mEdit);
    mHighlighter->setRegularExpression(mEdit->getRegEx());
    //splitter->addWidget(mEdit);

    mPreview = new QTextBrowser(this);
    mPreviewHighlighter = new NMParamHighlighter(mPreview);
    mPreviewHighlighter->setRegularExpression(mEdit->getRegEx());
    splitter->addWidget(mPreview);
    mPreview->hide();

    QWidget* widgetL = new QWidget(this);
    QVBoxLayout* vboxL = new QVBoxLayout(widgetL);
    mLabel = new QLabel(widgetL);
    vboxL->addWidget(mTreeWidget);
    vboxL->addWidget(mLabel);

    // ============================================
    // RIGHT HAND SIDE WIDGET
    // ============================================
    QWidget* widgetR = new QWidget(this);
    QVBoxLayout* vboxR = new QVBoxLayout(widgetR);

    QSplitter* splitterR = new QSplitter(Qt::Horizontal);
    splitterR->setSizePolicy(tepol);
    splitterR->addWidget(mEdit);
    splitterR->addWidget(mPreview);

    QHBoxLayout* hboxR = new QHBoxLayout();
    btnPreview = new QCheckBox(widgetR);
    btnPreview->setText("Preview Expression");
    btnPreview->setVisible(true);
    btnPreview->setCheckable(true);
    btnPreview->setChecked(false);
    hboxR->addWidget(btnPreview);
    hboxR->addStretch();
    mPosLabel = new QLabel(widgetR);
    hboxR->addWidget(mPosLabel);

    vboxR->addWidget(splitterR);
    vboxR->addItem(hboxR);

    QList<int> sizes;
    sizes << 40 << 120;

    splitter->addWidget(widgetL);
    splitter->addWidget(widgetR);
    splitter->setSizes(sizes);

    QHBoxLayout* bl2 = new QHBoxLayout();
    bl2->addStretch();

    QPushButton* btnapply = new QPushButton("Apply", this);
    bl2->addWidget(btnapply);

    QPushButton* btncancel = new QPushButton("Cancel", this);
    bl2->addWidget(btncancel);

    fl->addWidget(splitter);
    fl->addItem(bl2);


    connect(mTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            SLOT(updateModelItem(QTreeWidgetItem*,int)));
    connect(mTreeWidget, SIGNAL(maxTreeLevel(int)), SLOT(setTreeLevel(int)));
    connect(mEdit, &QTextEdit::cursorPositionChanged, this, &NMHoverEdit::assistEditing);
    connect(mEdit, SIGNAL(textChanged()), this, SLOT(updateExpressionPreview()));
    connect(btnPreview, SIGNAL(toggled(bool)), this, SLOT(showExpressionPreview(bool)));
    connect(btnapply, SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(btncancel, SIGNAL(clicked()), this, SLOT(close()));
}


void
NMHoverEdit::showExpressionPreview(bool preview)
{
    if (!preview)
    {
        mPreview->hide();
        return;
    }

    QString curExpr = mEdit->toPlainText();
    QString populatedExpr = NMModelController::getInstance()->processStringParameter(mComp, curExpr);
    mPreview->setText(populatedExpr);
    mPreview->show();
}

void
NMHoverEdit::assistEditing()
{
    QTextCursor cur = mEdit->textCursor();
    mPosLabel->setText(QString("pos: %1").arg(cur.position()));
}


void
NMHoverEdit::updateCompleter()
{
    QMap<QString, QString> compMap;
    QStringList userIds = NMModelController::getInstance()->getUserIDs();
    userIds.removeDuplicates();
    userIds.removeOne(QString(""));
    foreach(const QString& uid, userIds)
    {
        compMap.insert(uid, "UserID");
    }

    QStringList compNames = NMModelController::getInstance()->getRepository().keys();
    foreach(const QString& cn, compNames)
    {
        compMap.insert(cn, "ComponentName");
    }

    mEdit->setCompList(compMap, mComp);
    mEdit->setEditParameter(mPropName);
}

void
NMHoverEdit::showEvent(QShowEvent *event)
{
    updateCompleter();
}

void
NMHoverEdit::setProperty(const QString &compName, const QString& propName)
{
    NMModelComponent* comp = NMModelController::getInstance()->getComponent(compName);
    if (comp == 0)
    {
        if (mProc) this->disconnect(mProc);
        if (mComp) this->disconnect(mComp);
        mProc = 0;
        mComp = 0;
        mCompName.clear();
        mPropName.clear();
        mTreeWidget->setTreeModel(QVariant());
        mCurIndices.clear();
        mLabel->clear();
        mEdit->clear();
        return;
    }

    if (mProc)
    {
        disconnect(mProc, SIGNAL(nmChanged()), this, SLOT(updateEditor()));
        mProc = 0;
    }
    else if (mComp)
    {
        disconnect(mComp, SIGNAL(nmChanged()), this, SLOT(updateEditor()));
        mComp = 0;
    }

    mCompName.clear();
    mPropName.clear();
    mTreeWidget->clearCurrentIndices();
    mEdit->clear();

    if (comp)
    {
        if (NMModelController::getPropertyList(comp).contains(propName))
        {
            connect(comp, SIGNAL(nmChanged()), this, SLOT(updateEditor()));
            mComp = comp;
            mCompName = compName;
            mPropName = propName;
        }
        else
        {
            NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(comp);
            if (ic && ic->getProcess() != 0)
            {
                if (NMModelController::getPropertyList(ic->getProcess()).contains(propName))
                {
                    mProc = ic->getProcess();
                    mComp = comp;
                    connect(mProc, SIGNAL(nmChanged()), this, SLOT(updateEditor()));
                    mCompName = compName;
                    mPropName = propName;
                }
            }
        }
    }

    if (propName.compare("KernelScript") == 0)
    {
        mHighlighter->setExpressionType(NMParamHighlighter::NM_SCRIPT_EXP);
        mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_SCRIPT_EXP);
    }
    else if (propName.contains("SQL", Qt::CaseInsensitive))
    {
        mHighlighter->setExpressionType(NMParamHighlighter::NM_SQLITE_EXP);
        mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_SQLITE_EXP);
    }
    else if (propName.compare("MapExpressions") == 0)
    {
        mHighlighter->setExpressionType(NMParamHighlighter::NM_MATH_EXP);
        mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_MATH_EXP);
    }
    else
    {
        mHighlighter->setExpressionType(NMParamHighlighter::NM_PARM_EXP);
        mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_PARM_EXP);
    }

    updateEditor();
}

void
NMHoverEdit::updateEditor()
{
    if (mProc == 0 && mComp == 0)
    {
        return;
    }

    QVariant model;
    if (mProc)
    {
        model = mProc->property(mPropName.toStdString().c_str());
    }
    else if (mComp)
    {
        model = mComp->property(mPropName.toStdString().c_str());
    }

    mTreeWidget->setTreeModel(model);
    if (QString("QString").compare(model.typeName()) == 0)
    {
        mEdit->setText(model.toString());
    }

    QString title;
    if (mComp->getUserID().isEmpty())
    {
        title = QString("Edit %2 of %1").arg(mCompName).arg(mPropName);
    }
    else
    {
        title = QString("Edit %3 of %1 (%2)").arg(mComp->getUserID()).arg(mCompName).arg(mPropName);
    }
    this->setWindowTitle(title);

    mPreview->clear();
    mPreview->hide();
    btnPreview->setChecked(false);

    const int ml = mPropLevel;
    switch(ml)
    {
    case 0:
    case 1: mLabel->setText("Parameter"); break;
    case 2: mLabel->setText("List of Parameters"); break;
    case 3: mLabel->setText("List of List of Pameters"); break;
    }

    updateCompleter();

}

void
NMHoverEdit::updateModelItem(QTreeWidgetItem *item, int col)
{
    if (item == 0)
    {
        mEdit->clear();
        return;
    }

    // the easiest case
    QVariant model = mTreeWidget->getTreeModel();
    if (QString("QString").compare(model.typeName()) == 0)
    {
        mEdit->setText(model.toString());
        return;
    }

    QList<int> indices = this->mTreeWidget->getCurrentIndices();
    if (indices.size() < mPropLevel)
    {
        mEdit->setPlaceholderText("Select parameter");
        mEdit->clear();
        mEdit->setTextInteractionFlags(Qt::NoTextInteraction);
        return;
    }
    mEdit->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::LinksAccessibleByMouse);

    QString param;
    if (QString("QStringList").compare(model.typeName()) == 0)
    {
        if (indices.size() == 1)
        {
            param = model.toStringList().at(indices.at(0));
            if (param.isEmpty())
            {
                mEdit->setPlaceholderText("Edit parameter");
            }
            mEdit->setText(param);
        }
    }
    else if (QString("QList<QStringList>").compare(model.typeName()) == 0)
    {
        if (indices.size() == 2)
        {
            param = model.value<QList<QStringList> >().at(indices.at(0)).at(indices.at(1));
            if (param.isEmpty())
            {
                mEdit->setPlaceholderText("Edit parameter");
            }
            mEdit->setText(param);
        }
    }
    else if (QString("QList<QList<QStringList> >").compare(model.typeName()) == 0)
    {
        if (indices.size() == 3)
        {
            param = model.value<QList<QList<QStringList> > >().at(indices.at(0)).at(indices.at(1)).at(indices.at(2));
            if (param.isEmpty())
            {
                mEdit->setPlaceholderText("Edit parameter");
            }
            mEdit->setText(param);
        }
    }
    else
    {
        mEdit->setPlaceholderText("Edit parameter");
        mEdit->clear();
    }

    mCurIndices = indices;
}

void
NMHoverEdit::applyChanges()
{
    QVariant model = mTreeWidget->getTreeModel();
    QList<int> indices = mTreeWidget->getCurrentIndices();

    // if we're currently pointing at an individual parameter
    // we replace the model's value with the editor value
    if (indices.size() == mPropLevel)
    {
        QString newValue = mEdit->toPlainText().simplified();

        if (QString("QString").compare(model.typeName()) == 0)
        {
            model = QVariant::fromValue(newValue);
        }
        else if (indices.size() == 1)
        {
            QStringList sl = model.value<QStringList>();
            sl.replace(indices.at(0), newValue);
            model = QVariant::fromValue(sl);
        }
        else if (indices.size() == 2)
        {
            QList<QStringList> lsl = model.value<QList<QStringList> >();
            QStringList sl = lsl.at(indices.at(0));
            sl.replace(indices.at(1), newValue);
            lsl.replace(indices.at(0), sl);
            model = QVariant::fromValue(lsl);
        }
        else if (indices.size() == 3)
        {
            QList<QList<QStringList> > llsl = model.value<QList<QList<QStringList> > >();
            QList<QStringList> lsl = llsl.at(indices.at(0));
            QStringList sl = lsl.at(indices.at(1));
            sl.replace(indices.at(2), newValue);
            lsl.replace(indices.at(1), sl);
            llsl.replace(indices.at(0), lsl);
            model = QVariant::fromValue(llsl);
        }
    }

    // transfer model to the underlying component
    if (mProc)
    {
        mProc->setProperty(mPropName.toStdString().c_str(), model);
    }
    else if (mComp)
    {
        mComp->setProperty(mPropName.toStdString().c_str(), model);
    }
}
