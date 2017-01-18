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

#ifndef NMPARAMEDIT_H
#define NMPARAMEDIT_H

#include <QTextEdit>
#include <QRegularExpressionMatchIterator>

class QCompleter;
class QAbstractItemModel;
class NMModelComponent;
class QItemSelection;

class NMParamEdit : public QTextEdit
{
    Q_OBJECT
public:
    NMParamEdit(QWidget* parent=0);

    void setCompList(QMap<QString, QString> compMap,
                     NMModelComponent* editComp);
    void setEditParameter(const QString& param)
        {mEditParameter = param;}
    QRegularExpression getRegEx(void)
        {return mRegEx;}

signals:

public slots:

protected slots:
    void insertCompletion(const QString& completion);
    void showContextInfo(const QItemSelection& sel);

protected:

    enum CompletionMode
    {
        NM_NO_COMPLETION = 0,
        NM_COMPNAME_COMPLETION,
        NM_PROPNAME_COMPLETION,
        NM_COLVALUE_COMPLETION
    };

    void keyPressEvent(QKeyEvent* e);
    NMModelComponent* getModelComponent(const QString& compName);
    CompletionMode setupValueCompleter(const QString& compName,
                           const QString& propName);
    CompletionMode setupPropCompleter(const QString& comp, int propPos, bool dataOnly=false);
    void processCompletion(void);
    QString textUnderCursor();


private:
    CompletionMode mCompletionMode;
    int mPropPos;

    NMModelComponent* mEditComp;
    QStringList mCompList;
    QMap<QString, QString> mCompToolTipMap;
    QMap<QString, QString> mPropToolTipMap;
    QMap<QString, int> mValueIdxMap;

    QCompleter* mCompleter;
    QStringList mPropBlackList;
    QStringList mMuParserParameterList;

    QString mLastTip;
    QString mEditParameter;

    QRegularExpression mRegEx;
    QRegularExpressionMatchIterator mRexIt;
};

#endif // NMPARAMEDIT_H
