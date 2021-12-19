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
#include <QKeyEvent>

#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "NMComponentEditor.h"
#include "NMModelController.h"
#include "NMGlobalHelper.h"
#include "NMIterableComponent.h"
#include "NMHoverEditTree.h"
#include "NMParamHighlighter.h"
#include "NMParamEdit.h"
#include "NMFindReplaceDialog.h"
#include "NMMfwException.h"


NMHoverEdit::NMHoverEdit(QWidget *parent)
    : QDialog(parent), mComp(0), mProc(0), mPropLevel(0),
      mFindReplaceDlg(0)
{
    this->setModal(false);

    // ==================================================
    //      LAYOUT VISIBLE CONTROLS
    // ==================================================
    QFormLayout* fl = new QFormLayout(this);

    QSizePolicy tepol(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tepol.setVerticalStretch(1);

    mMainSplitter = new QSplitter(Qt::Horizontal);
    mMainSplitter->setSizePolicy(tepol);

    mTreeWidget = new NMHoverEditTree(this);
    mTreeWidget->setObjectName("treeWidget");
    mTreeWidget->setDragEnabled(true);
    mTreeWidget->setTreeModel(QVariant());

    mEdit = new NMParamEdit(this);
    mEdit->setObjectName("ParamEditor");
    mEdit->setSizePolicy(tepol);
    mEdit->setPlaceholderText("Select a parameter");
    mEdit->installEventFilter(this);
    mHighlighter = new NMParamHighlighter(mEdit);
    //mHighlighter->setRegularExpression(mEdit->getRegEx());

    mPreview = new QTextBrowser(this);
    mPreviewHighlighter = new NMParamHighlighter(mPreview);
    //mPreviewHighlighter->setRegularExpression(mEdit->getRegEx());
    //splitter->addWidget(mPreview);
    mPreview->hide();

    // adapt highlighter colours for dark mode, if applicable
    QColor bkg = this->mEdit->palette().background().color();
    bool bdark = false;
    if (    NMGlobalHelper::getMainWindow()->isInDarkMode()
         || (bkg.red() < 80 && bkg.green() < 80 && bkg.blue() < 80)
       )
    {
        bdark = true;
    }

    mHighlighter->setDarkColorMode(bdark);
    mPreviewHighlighter->setDarkColorMode(bdark);


    QWidget* widgetL = new QWidget(this);
    QVBoxLayout* vboxL = new QVBoxLayout(widgetL);
    mLabel = new QLabel(widgetL);
    vboxL->addWidget(mTreeWidget);
    //vboxL->addWidget(mLabel);

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

    mSplitterSizes.clear();
    mSplitterSizes << 40 << 120;

    mMainSplitter->addWidget(widgetL);
    mMainSplitter->addWidget(widgetR);
    mMainSplitter->setSizes(mSplitterSizes);

    QHBoxLayout* bl2 = new QHBoxLayout();
    bl2->addStretch();

    QPushButton* btnapply = new QPushButton("Apply", this);
    bl2->addWidget(btnapply);

    QPushButton* btncancel = new QPushButton("Cancel", this);
    bl2->addWidget(btncancel);

    fl->addWidget(mLabel);
    fl->addWidget(mMainSplitter);
    fl->addItem(bl2);

    connect(mMainSplitter, SIGNAL(splitterMoved(int,int)),
            this, SLOT(storeSplitterSize(int,int)));

    connect(mTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            SLOT(updateModelItem(QTreeWidgetItem*,int)));
    connect(mTreeWidget, SIGNAL(maxTreeLevel(int)), SLOT(setTreeLevel(int)));
    connect(mEdit, &QTextEdit::cursorPositionChanged, this, &NMHoverEdit::assistEditing);
    connect(mEdit, SIGNAL(textChanged()), this, SLOT(updateExpressionPreview()));
    connect(btnPreview, SIGNAL(toggled(bool)), this, SLOT(showExpressionPreview(bool)));
    connect(btnapply, SIGNAL(clicked()), this, SLOT(applyChanges()));
    connect(btncancel, SIGNAL(clicked()), this, SLOT(close()));

    this->setTabOrder(mTreeWidget, mEdit);
    this->setTabOrder(mEdit, btnPreview);
    this->setTabOrder(btnPreview, btnapply);
    this->setTabOrder(btnapply, btncancel);
}

void
NMHoverEdit::closeEvent(QCloseEvent *event)
{
    if (mFindReplaceDlg)
    {
        mFindReplaceDlg->close();
    }
    QDialog::closeEvent(event);
}

void
NMHoverEdit::forwardModelConfigChanged(void)
{
    if (btnPreview->isChecked())
    {
        this->showExpressionPreview(true);
    }
}

bool
NMHoverEdit::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mEdit)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->modifiers() & Qt::ControlModifier && ke->key() == Qt::Key_F)
            {
                if (mFindReplaceDlg == 0)
                {
                    mFindReplaceDlg = new NMFindReplaceDialog(mEdit);
                    mFindReplaceDlg->setWindowTitle(tr("Find and Replace"));
                }
                mFindReplaceDlg->setEnabled(true);
                mFindReplaceDlg->show();
                return true;
            }
//            else if (ke->key() == Qt::Key_Tab)
//            {
//                if (!mEdit->hasFocus())
//                {
//                    btnPreview->setFocus();
//                    return true;
//                }
//            }
//            else if (ke->key() == Qt::RightArrow)
//            {
//                btnPreview->setFocus();
//                return true;
//            }
        }
    }
    return false;
}

void
NMHoverEdit::showExpressionPreview(bool preview)
{
    NMModelController* ctrl = NMGlobalHelper::getModelController();


    if (!preview || ctrl == 0)
    {
        mPreview->hide();
        return;
    }

    QString curExpr = mEdit->toPlainText();

    // iterate over each expression in the parameter and substitute with
    // the evaluated value
    const int maxcount = 15000;

    int count = 0;
    QStringList expList = ctrl->getNextParamExpr(curExpr);
    while (expList.size() > 0)
    {
        if (count >= maxcount)
        {
            NMLogError(<< "The maximum allowed number of "
                       << "expressions per parameter "
                       << "has been exceeded. Note: LUMASS"
                       << "currently only supports a depth"
                          "recursion level of 10 for "
                       << "parameter expressions!")

            break;
        }

        for (int i=0; i < expList.size(); ++i)
        {
            QString rawStr = expList.at(i);
            QString evalExpr;
            try
            {
                evalExpr = ctrl->processStringParameter(mComp, rawStr);
            }
            catch(NMMfwException& me)
            {
                //NMLogError(<< me.what());
                evalExpr = QString("ERROR: %1").arg(evalExpr);
            }

            curExpr = curExpr.replace(rawStr, evalExpr);
            if (evalExpr.startsWith("ERROR"))
            {
                mPreview->setText(curExpr);
                mPreview->show();
                return;
            }
            else
            {
                ++count;
            }
        }
        expList = ctrl->getNextParamExpr(curExpr);
    }

    mPreview->setText(curExpr);
    mPreview->show();
}

void
NMHoverEdit::updateExpressionPreview()
{
    if (btnPreview->isChecked())
    {
        showExpressionPreview(true);
    }
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
    QStringList userIds = NMGlobalHelper::getModelController()->getUserIDs();
    userIds.removeDuplicates();
    userIds.removeOne(QString(""));
    foreach(const QString& uid, userIds)
    {
        compMap.insert(uid, "UserID");
    }

    QStringList compNames = NMGlobalHelper::getModelController()->getRepository().keys();
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
    NMModelController* ctrl = NMGlobalHelper::getModelController();
    NMModelComponent* comp = ctrl->getComponent(compName);
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
        if (ctrl->getPropertyList(comp).contains(propName))
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
                mProc = ic->getProcess();
                QString internalPropName = mProc->mapDisplayToPropertyName(propName);

                if (ctrl->getPropertyList(ic->getProcess()).contains(internalPropName))
                {
                    mProc = ic->getProcess();
                    mComp = comp;
                    connect(mProc, SIGNAL(nmChanged()), this, SLOT(updateEditor()));
                    mCompName = compName;
                    mPropName = internalPropName;
                }
                else
                {
                    mProc = nullptr;
                }
            }
        }
    }

    if (propName.compare("KernelScript") == 0 || propName.compare("InitScript") == 0)
    {
        if (comp->objectName().startsWith(QStringLiteral("JSMapKernelScript")))
        {
            mHighlighter->setExpressionType(NMParamHighlighter::NM_JS_EXP);
            mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_JS_EXP);
        }
        else
        {
            mHighlighter->setExpressionType(NMParamHighlighter::NM_SCRIPT_EXP);
            mPreviewHighlighter->setExpressionType(NMParamHighlighter::NM_SCRIPT_EXP);
        }
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
    QString displayName = mPropName;
    if (mProc)
    {
        model = mProc->property(mPropName.toStdString().c_str());
        displayName = mProc->getUserProperty(mPropName);
    }
    else if (mComp)
    {
        model = mComp->property(mPropName.toStdString().c_str());
    }

    mTreeWidget->setTreeModel(model);
    if (QString("QString").compare(model.typeName()) == 0)
    {
        mEdit->setPlaceholderText(tr("Edit parameter"));
        mEdit->setText(model.toString());

        mTreeWidget->setVisible(false);
        QList<int> nsizes;
        nsizes << 0 << (mSplitterSizes[0] + mSplitterSizes[1]);
        mMainSplitter->setSizes(nsizes);
    }
    else
    {
        if (!mTreeWidget->isVisible())
        {
            mTreeWidget->setVisible(true);
            QList<int> nsizes;
            nsizes << 40 << (mSplitterSizes[1]-40);
            mMainSplitter->setSizes(nsizes);
        }
    }

    QString title;
    if (mComp->getUserID().isEmpty())
    {
        title = QString("Edit %2 of %1").arg(mCompName).arg(displayName);
    }
    else
    {
        title = QString("Edit %3 of %1 (%2)").arg(mComp->getUserID()).arg(mCompName).arg(displayName);
    }
    this->setWindowTitle(title);

    const int ml = mPropLevel;
    switch(ml)
    {
    case 0: mLabel->setText("Single parameter for all iterations"); break;
    case 1: mLabel->setText("One parameter per iteration"); break;
    case 2: mLabel->setText("List of parameters per iteration"); break;
    case 3: mLabel->setText("List of list of pameters per iteration"); break;
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
        mEdit->setText(unquoteParam(model.toString()));
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
            mEdit->setText(unquoteParam(param));
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
            mEdit->setText(unquoteParam(param));
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
            mEdit->setText(unquoteParam(param));
        }
    }
    else
    {
        mEdit->setPlaceholderText("Edit parameter");
        mEdit->clear();
    }

    mCurIndices = indices;
}

bool
NMHoverEdit::containsUnquotedCurlyBrace(const QString &param)
{
    if (param.isEmpty())
    {
        return false;
    }

    int bfound = false;
    int pos = -1;

    for (int i=0; i < param.size(); ++i)
    {
        if ( (    param[i] == '{'
              ||  param[i] == '}'
             )
             &&  pos < 0
           )
        {
            bfound = true;
            i = param.size();
        }
        else if (param[i] == '\"')
        {
            if (pos < 0)
            {
                pos = i;
            }
            else
            {
                pos = -1;
            }
        }
    }

    return bfound;
}

QString
NMHoverEdit::quoteParam(const QString &param)
{
    QString ret = param;

    if (containsUnquotedCurlyBrace(param))
    {
        ret = QString("\"%1\"").arg(param);
    }

    return ret;
}

QString
NMHoverEdit::unquoteParam(const QString &param)
{
    if (param.isEmpty())
    {
        return param;
    }

    // remove outermost quotations
    QString ret = param;
    int cnt = param.count('\"');
    if (    param.startsWith('\"')
        &&  param.endsWith('\"')
        &&  cnt == 2
       )
    {
        ret = ret.mid(1);
        ret = ret.left(ret.size()-1);
    }

    // test for unquoted curly braces
    if (!containsUnquotedCurlyBrace(ret))
    {
        ret = param;
    }

    return ret;
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
        QString newValue = quoteParam(mEdit->toPlainText());//.simplified();

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
    bool bchanged = false;
    if (mProc)
    {
        QVariant oldProp = mProc->property(mPropName.toStdString().c_str());
        if (oldProp != model)
        {
            mProc->setProperty(mPropName.toStdString().c_str(), model);
            bchanged = true;
        }
    }
    else if (mComp)
    {
        QVariant oldProp = mComp->property(mPropName.toStdString().c_str());
        if (oldProp != model)
        {
            mComp->setProperty(mPropName.toStdString().c_str(), model);
            bchanged = true;
        }
    }

    if (bchanged)
    {
        emit signalCompProcChanged();
    }

    updateEditor();
}
