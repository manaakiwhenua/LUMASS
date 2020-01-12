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

#ifndef __NMHOVEREDIT_
#define __NMHOVEREDIT_

#include <QDialog>
#include <QLabel>
#include <QSplitter>

class QCheckBox;
class QTreeWidgetItem;
class QListWidget;
class QtProperty;
class QCompleter;
class QTextBrowser;
class NMParamEdit;
class NMLogger;
class NMHoverEditTree;
class NMModelComponent;
class NMProcess;
class NMComponentEditor;
class NMParamHighlighter;
class NMFindReplaceDialog;

class NMHoverEdit : public QDialog
{
    Q_OBJECT

public:
    NMHoverEdit(QWidget* parent=0);
    ~NMHoverEdit(){}

    void setProperty(const QString& compName,
                     const QString& propName);

    QString getComponentName(void) {return mCompName;}
    QString getPropertyName(void) {return mPropName;}

    void setLogger(NMLogger* logger) {mLogger = logger;}

signals:
    void signalCompProcChanged();

public slots:


protected slots:
    void applyChanges();
    void updateEditor();
    void setTreeLevel(int level){mPropLevel = level;}
    void updateModelItem(QTreeWidgetItem* item, int col);
    void assistEditing();
    void updateExpressionPreview();
    void showExpressionPreview(bool preview);
    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent *event);
    void storeSplitterSize(int pos, int idx)
        {mSplitterSizes = mMainSplitter->sizes();}

protected:
    void showEvent(QShowEvent* event);
    void updateCompleter();
    bool containsUnquotedCurlyBrace(const QString& param);
    QString quoteParam(const QString& param);
    QString unquoteParam(const QString& param);


private:
    QString mCompName;
    QString mPropName;
    int mPropLevel;

    QList<int> mSplitterSizes;
    QSplitter* mMainSplitter;
    NMParamEdit* mEdit;
    QTextBrowser* mPreview;
    NMParamHighlighter* mHighlighter;
    NMParamHighlighter* mPreviewHighlighter;
    NMHoverEditTree* mTreeWidget;
    NMFindReplaceDialog* mFindReplaceDlg;

    NMModelComponent* mComp;
    NMProcess* mProc;

    //QPushButton* btnPreview;
    QCheckBox* btnPreview;
    QLabel* mLabel;
    QLabel* mPosLabel;

    NMLogger* mLogger;

    QList<int> mCurIndices;

};

#endif // __NMHOVEREDIT_
