/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2017 Landcare Research New Zealand Ltd
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

#ifndef NMFINDREPLACEDIALOG_H
#define NMFINDREPLACEDIALOG_H

#include <QDialog>
#include <QTextEdit>

class QCheckBox;

class NMFindReplaceDialog : public QDialog
{
    Q_OBJECT
public:
    NMFindReplaceDialog(QTextEdit* parent);
    ~NMFindReplaceDialog(){}

signals:

public slots:

protected slots:
    void findreplace();
    void updateApplyButton();
    bool eventFilter(QObject *obj, QEvent *event);

protected:

private:
    QTextEdit* mTextEdit;
    QTextEdit* mFindText;
    QTextEdit* mReplaceText;

    QCheckBox* mUseRegEx;
    QCheckBox* mUseOnlySelectedText;

    QPushButton* mBtnApply;

};

#endif // NMFINDREPLACEDIALOG_H
