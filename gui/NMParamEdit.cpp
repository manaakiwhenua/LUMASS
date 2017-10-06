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

#include "NMParamEdit.h"

#include <QCompleter>
#include <QEvent>
#include <QKeyEvent>
#include <QCompleter>
#include <QScrollBar>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QAbstractItemView>
#include <QDebug>
#include <QStringListModel>
#include <QToolTip>

#include "NMModelController.h"
#include "NMModelComponent.h"
#include "NMIterableComponent.h"
#include "NMDataComponent.h"
#include "NMGlobalHelper.h"

NMParamEdit::NMParamEdit(QWidget *parent)
    : QTextEdit(parent), mEditComp(0), mCompletionMode(NM_COMPNAME_COMPLETION),
      mPropPos(-1)
{
    mCompleter = new QCompleter(this);
    mCompleter->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    mCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mCompleter->setWrapAround(false);
    mCompleter->setWidget(this);
    mCompleter->setCompletionMode(QCompleter::PopupCompletion);

    connect(mCompleter, SIGNAL(activated(QString)), this,
            SLOT(insertCompletion(QString)));
    connect(mCompleter->popup()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(showContextInfo(QItemSelection)));

    mPropBlackList << "objectName" << "Description" << "Inputs"
                   << "InputComponents"
                   << "ParameterHandling" << "NMInputComponentType"
                   << "NMOutputComponentType" << "InputNumDimensions"
                   << "OutputNumDimensions" << "InputNumBands"
                   << "OutputNumBands";

    mMuParserParameterList << "MapExpressions" << "KernelScript";


    mRegEx = QRegularExpression("((?<open>\\$\\[)*"
                               "(?(<open>)|\\b)"
                               "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
                               "(?<sep1>(?(<open>):|(?>__)))*"
                               "(?<arith>(?(<sep1>)|([ ]*(?<opr>[+\\-])?[ ]*(?<sum>[\\d]+))))*"
                               "(?<prop>(?(?<!math:|func:)(?(<sep1>)\\g<comp>)|([a-zA-Z0-9_ \\/\\(\\)&%\\|\\>\\!\\=\\<\\-\\+\\*\\^\\?:;.,'\"])*))*"
                               "(?<sep2>(?(<prop>)(?(<open>):)))*"
                               "(?(<sep2>)(?<idx>[0-9]+)|([ ]*(?<opr2>[+\\-]+)[ ]*(?<sum2>[\\d]+))*))(?>\\]\\$)*");


//            QRegularExpression("((?<open>\\$\\[)*"
//                             "(?(<open>)|\\b)"
//                             "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
//                             "(?<sep1>(?(<open>):|(?>__)))*"
//                             "(?<prop>(?(<sep1>)\\g<comp>))*"
//                             "(?<sep2>(?(<prop>)(?(<open>)):))*"
//                             "(?<idx>(?(<sep2>)[\\w]*)))");

    updateWhiteSpaceTab();
}

NMModelComponent*
NMParamEdit::getModelComponent(const QString& compName)
{
    NMModelComponent* pcomp = NMGlobalHelper::getModelController()->getComponent(compName);
    if (pcomp == 0 && mEditComp)
    {
        NMIterableComponent* pic = qobject_cast<NMIterableComponent*>(mEditComp);
        if (pic == 0)
        {
            if (mEditComp->getHostComponent())
            {
                pic = qobject_cast<NMIterableComponent*>(mEditComp->getHostComponent());
            }
        }

        if (pic)
        {
            pcomp = pic->findUpstreamComponentByUserId(compName);
        }
    }
    return pcomp;
}

void
NMParamEdit::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->delta() > 0)
        {
            this->zoomIn(2);
        }
        else
        {
            this->zoomOut(2);
        }
        updateWhiteSpaceTab();
    }
}

void
NMParamEdit::updateWhiteSpaceTab()
{
    // set the default tabStopWidth
    int fw = QFontMetrics(this->currentCharFormat().font()).width(QLatin1Char(' '));
    this->setTabStopWidth(fw * 4);
}

void
NMParamEdit::showContextInfo(const QItemSelection& sel)
{
    if (!mCompleter->popup()->isHidden()
        && sel.count()
        && mCompletionMode != NM_NO_COMPLETION)
    {
        const int id = sel.at(0).top();
        QRect listrect = mCompleter->popup()->rect();
        QModelIndex idx = mCompleter->model()->index(id,0);
        QModelIndex tidx = mCompleter->popup()->indexAt(listrect.topLeft());

        const int relpos = idx.row() - tidx.row();
        const int rowHeight = mCompleter->popup()->sizeHintForRow(0);
        int top = relpos * rowHeight;
        if (top < 0)
        {
            top = 0;
        }
        else if (top > listrect.bottom()-rowHeight)
        {
            top = listrect.bottom()-rowHeight;
        }

        // neutralise platform specific tooltip offset
        #ifdef Q_WS_WIN
            top -= 21;
        #else
            top -= 16;
        #endif

        QPoint tpos(mCompleter->popup()->sizeHintForColumn(0)
                  + mCompleter->popup()->verticalScrollBar()->sizeHint().width(),
                  top);

        QString tip;
        QString completion = mCompleter->model()->data(idx).toString();
        QMap<QString, QString>::iterator mit;
        if (mCompletionMode == NM_PROPNAME_COMPLETION)
        {
            mit = mPropToolTipMap.find(completion);
            if (mit != mPropToolTipMap.end())
            {
                tip = mit.value();
            }
        }
        else
        {
            mit = mCompToolTipMap.find(completion);
            if (mit != mCompToolTipMap.end())
            {
                tip = mit.value();
            }
        }

        if (tip.compare(mLastTip) == 0)
        {
            QToolTip::showText(mCompleter->popup()->mapToGlobal(tpos), QString("%1 ").arg(tip));
        }
        QToolTip::showText(mCompleter->popup()->mapToGlobal(tpos), tip, 0, QRect());
        mLastTip = tip;
    }
}

void
NMParamEdit::setCompList(QMap<QString, QString> compMap,
                         NMModelComponent* editComp)
{
    mCompList = compMap.keys();
    mCompList.sort(Qt::CaseInsensitive);
    mCompToolTipMap = compMap;
    mCompleter->setModel(new QStringListModel(mCompList, mCompleter));
    mEditComp = editComp;
}

void
NMParamEdit::keyPressEvent(QKeyEvent *e)
{
    if (mCompleter && mCompleter->popup()->isVisible())
    {
        // The following keys are forwarded by the completer to the widget
        switch (e->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            e->ignore();
            return; // let the completer do default behavior
        case Qt::Key_Tab:
        case Qt::Key_Escape:
            if (    mCompletionMode == NM_PROPNAME_COMPLETION
                ||  mCompletionMode == NM_COLVALUE_COMPLETION)
            {
                setupValueCompleter(QString(), QString());
                setupPropCompleter(QString(), -1);
                mCompleter->popup()->hide();
            }
            e->ignore();
            return;
        default:
            break;
        }
    }

    QTextEdit::keyPressEvent(e);


    if ((e->text().isEmpty()
        || e->key() == Qt::Key_Backspace
        || e->key() == Qt::Key_Backtab)
        //&& e->key() != QKeySequence(Qt::ALT + Qt::Key_C))
            )
    {
        mCompletionMode = NM_NO_COMPLETION;
        mCompleter->popup()->hide();
        return;
    }

    QString text = this->toPlainText();
    int cpos = this->textCursor().position();
    mRexIt = mRegEx.globalMatch(text);

    NMParamEdit::CompletionMode completionMode = NM_NO_COMPLETION;
    while (mRexIt.hasNext())
    {
        QRegularExpressionMatch match = mRexIt.next();
        QString tmpStr = match.captured(0);
        int spos = match.capturedStart(0);
        int epos = match.capturedEnd(0);
        int len = match.capturedLength(0);
        qDebug() << "cpos=" << cpos << "match: " << tmpStr << " (" << spos << "," << epos << ")";


        if (cpos >= spos && cpos <= epos)
        {
            QStringRef open = match.capturedRef("open");
            QStringRef comp = match.capturedRef("comp");
            QStringRef sep1 = match.capturedRef("sep1");
            QStringRef prop = match.capturedRef("prop");
            QStringRef sep2 = match.capturedRef("sep2");
            QStringRef idx = match.capturedRef("idx");
            qDebug() << "comp=" << comp.position()
                     << " sep1=" << (sep1.isEmpty() ? -1 : sep1.position())
                     << " prop=" << prop.position()
                     << " sep2=" << (sep2.isEmpty() ? -1 : sep2.position());


            // Col value completion
            if (!sep2.isEmpty() && cpos > sep2.position())
            {
                // $[mycomp:myprop:index]$
                // cat__myprop
                completionMode = NM_COLVALUE_COMPLETION;
                if (mCompletionMode != NM_COLVALUE_COMPLETION)
                {
                    setupValueCompleter(comp.toString(), prop.toString());
                }

                QString prefix;
                if (!idx.isEmpty())
                {
                    prefix = idx.mid(0, cpos-idx.position()).toString();
                }
                else
                {
                    prefix = tmpStr.mid(sep2.position()+1, epos-sep2.position());
                }
                mCompleter->setCompletionPrefix(prefix);
                qDebug() << "value completion prefix: " << prefix;
                //mCompleter->setCompletionPrefix(QString(""));
            }
            // Prop completion
            else if (   !sep1.isEmpty() && cpos >= sep1.position() + sep1.length()
                     && !(!sep2.isEmpty() && cpos > sep2.position()))
            {
                bool bOnlyData = false;
                if (sep1.compare(QString("__")) == 0)
                {
                    if (!mMuParserParameterList.contains(mEditParameter))
                    {
                        break;
                    }
                    bOnlyData = true;
                }
                else if (sep1.compare(QString(":")) == 0 && open.isEmpty())
                {
                    break;
                }

                completionMode = NM_PROPNAME_COMPLETION;
                if (mCompletionMode != NM_PROPNAME_COMPLETION)
                {
                    //qDebug() << "setup propCompleter";
                    completionMode = setupPropCompleter(comp.toString(), cpos, bOnlyData);
                }

                if (prop.isEmpty())
                {
                    mCompleter->setCompletionPrefix(QString(""));
                    //qDebug() << "prop completion prefix: \n";
                }
                else
                {
                    mCompleter->setCompletionPrefix(prop.mid(0, cpos-prop.position()).toString());
                    //qDebug() << "prop completion prefix: " << prop.mid(0, cpos-prop.position()).toString() << "\n";
                }
            }
            // Comp completion
            else if (!comp.isEmpty() && cpos > comp.position() && cpos <= comp.position()+comp.length())
            {
                completionMode = NM_COMPNAME_COMPLETION;
                if (mCompletionMode != NM_COMPNAME_COMPLETION)
                {
                    //qDebug() << "setup compCompleter";
                    mCompleter->setModel(new QStringListModel(mCompList, mCompleter));
                }
                mCompleter->setCompletionPrefix(comp.mid(0, cpos-comp.position()).toString());
                //qDebug() << "comp completion prefix: " << comp.mid(0, cpos-comp.position()).toString() << "\n";
            }
            break;
        }
    }
    mCompletionMode = completionMode;
    qDebug() << "completion mode: " << completionMode;

    if (mCompletionMode != NM_NO_COMPLETION)
    {
        mCompleter->popup()->setCurrentIndex(mCompleter->completionModel()->index(0, 0));
        QRect cr = cursorRect();
        cr.setWidth(mCompleter->popup()->sizeHintForColumn(0)
                    + mCompleter->popup()->verticalScrollBar()->sizeHint().width());
        //qDebug() << "COMPLETE!\n";
        mCompleter->complete(cr);
    }
}

void
NMParamEdit::processCompletion(void)
{

}

NMParamEdit::CompletionMode
NMParamEdit::setupValueCompleter(const QString &compName, const QString &propName)
{
    bool bFailed = true;
    NMModelComponent* mc = this->getModelComponent(compName);
    if (mc)
    {
        otb::AttributeTable::Pointer tab = NMGlobalHelper::getModelController()->getComponentTable(mc);
        if (tab.IsNotNull())
        {
            int colid = tab->ColumnExists(propName.toStdString());
            if (colid >= 0)
            {
                int nrows = tab->GetNumRows();
                if (nrows > 256)
                {
                    nrows = 256;
                }

                QStringList values;
                mValueIdxMap.clear();
                for (int i=0; i < nrows; ++i)
                {
                    QString v = tab->GetStrValue(colid, i).c_str();
                    if (!v.isEmpty())
                    {
                        values << v;
                        mValueIdxMap.insert(v, i+1);
                    }
                }

                values.removeDuplicates();
                values.sort(Qt::CaseInsensitive);
                mCompleter->setModel(new QStringListModel(values, mCompleter));
                bFailed = false;
            }
        }
    }
    else if (compName.compare(QString("LUMASS"), Qt::CaseInsensitive) == 0)
    {
        mValueIdxMap.clear();

        QStringList keys = NMGlobalHelper::getUserSettingsList();
        if (keys.contains(propName))
        {
            QStringList values;
            values << NMGlobalHelper::getUserSetting(propName);
            mValueIdxMap.insert(values.at(0), 1);
            mCompleter->setModel(new QStringListModel(values, mCompleter));
            bFailed = false;
        }
    }

    // reset to component completion if we couldn't fetch
    // any values for this comp / prop combination
    if (bFailed)
    {
        mValueIdxMap.clear();
        return NM_NO_COMPLETION;
    }

    return NM_COLVALUE_COMPLETION;
}

NMParamEdit::CompletionMode
NMParamEdit::setupPropCompleter(const QString &comp, int propPos, bool dataOnly)
{
    // reset to comp completer
    if (comp.isEmpty() && propPos < 0)
    {
        mPropPos = -1;
        mPropToolTipMap.clear();
        return NM_NO_COMPLETION;
    }
    // set up property completion
    else if (!comp.isEmpty() && propPos > 0)
    {
        NMModelController* ctrl = NMGlobalHelper::getModelController();
        NMModelComponent* pcomp = ctrl->getComponent(comp);
        QStringList propList;
        mPropToolTipMap.clear();
        if (pcomp)
        {
            QStringList dataProps = NMGlobalHelper::getModelController()->getDataComponentProperties(pcomp);
            foreach(const QString& dp, dataProps)
            {
                mPropToolTipMap.insert(dp, "TableColumn");
            }
            propList << dataProps;

            if (!dataOnly)
            {
                NMIterableComponent* ic = qobject_cast<NMIterableComponent*>(pcomp);
                if (ic && ic->getProcess())
                {
                    QStringList procProps = ctrl->getPropertyList(ic->getProcess());
                    procProps.removeDuplicates();
                    foreach(const QString& pp, procProps)
                    {
                        mPropToolTipMap.insert(pp, "ProcessParameter");
                    }
                    propList << procProps;
                }
                QStringList compProps = ctrl->getPropertyList(pcomp);
                compProps.removeDuplicates();
                foreach(const QString& cp, compProps)
                {
                    mPropToolTipMap.insert(cp, "ComponentProperty");
                }
                propList << compProps;
            }

            // filter properties through blacklist
            foreach(const QString& bl, mPropBlackList)
            {
                if (propList.contains(bl))
                {
                    mPropToolTipMap.remove(bl);
                    propList.removeAll(bl);
                }
            }
        }
        else if (comp.compare(QString("LUMASS"), Qt::CaseInsensitive) == 0)
        {
            QStringList keys = NMGlobalHelper::getUserSettingsList();
            foreach (const QString& key, keys)
            {
                propList.append(key);
                mPropToolTipMap.insert(key, "LUMASS Setting");
            }
        }
        else
        {
            return NM_NO_COMPLETION;
        }

        propList.sort(Qt::CaseInsensitive);
        mPropPos = propPos;
        QStringListModel* slm = new QStringListModel(propList, mCompleter);
        if (slm)
        {
            //mCompleter->setModel(new QStringListModel(propList, mCompleter));
            mCompleter->setModel(slm);
        }
        else
        {
            return NM_NO_COMPLETION;
        }
        return NM_PROPNAME_COMPLETION;
    }
    // just update the completionPrefix
    else if (propPos > 0 && mPropPos > 0)
    {
        QTextCursor tc = this->textCursor();
        tc.setPosition(mPropPos);
        tc.setPosition(propPos, QTextCursor::KeepAnchor);

        mCompleter->setCompletionPrefix(tc.selectedText());
    }

    return NM_PROPNAME_COMPLETION;
}

QString
NMParamEdit::textUnderCursor()
{
    QTextCursor cur = this->textCursor();
    cur.select(QTextCursor::WordUnderCursor);
    return cur.selectedText();
}

void
NMParamEdit::insertCompletion(const QString &completion)
{
    QString theCompletion = completion;
    if (mCompletionMode == NM_COLVALUE_COMPLETION)
    {
        QMap<QString, int>::iterator mit;
        mit = mValueIdxMap.find(completion);
        if (mit != mValueIdxMap.end())
        {
            theCompletion = QString("%1").arg(mit.value());
        }
    }

    QTextCursor cur = this->textCursor();
    int cpos = cur.position();
    cur.movePosition(QTextCursor::Left,
                     QTextCursor::MoveAnchor,
                     mCompleter->completionPrefix().length());
    cur.setPosition(cpos, QTextCursor::KeepAnchor);
    cur.insertText(theCompletion);
    this->setTextCursor(cur);

    if (mCompletionMode == NM_COLVALUE_COMPLETION)
    {
        setupValueCompleter(QString(), QString());
    }
    else if (mCompletionMode == NM_PROPNAME_COMPLETION)
    {
        setupPropCompleter(QString(), -1);
    }
    mCompletionMode = NM_NO_COMPLETION;
}








