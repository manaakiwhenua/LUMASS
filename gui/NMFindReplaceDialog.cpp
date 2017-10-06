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

#include "NMFindReplaceDialog.h"

#include <QCheckBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QTextCursor>
#include <QLabel>
#include <QEvent>
#include <QKeyEvent>

NMFindReplaceDialog::NMFindReplaceDialog(QTextEdit* parent)
    : QDialog(parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QFormLayout* formLayout = new QFormLayout(this);

    mTextEdit = qobject_cast<QTextEdit*>(parent);

    mFindText = new QTextEdit(this);
    mFindText->setFixedHeight(80);
    mFindText->installEventFilter(this);
    mFindText->setEnabled(true);
    mReplaceText = new QTextEdit(this);
    mReplaceText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mReplaceText->setFixedHeight(80);
    mReplaceText->installEventFilter(this);
    mReplaceText->setEnabled(true);

    QHBoxLayout* boxOptions = new QHBoxLayout();
    mUseOnlySelectedText = new QCheckBox(tr("Selected Text"), this);
    mUseOnlySelectedText->setChecked(false);
    mUseRegEx = new QCheckBox(tr("Use Regular Expression (PCRE)"), this);
    mUseRegEx->setChecked(false);
    boxOptions->addWidget(mUseOnlySelectedText);
    boxOptions->addWidget(mUseRegEx);

    QHBoxLayout* boxButtons = new QHBoxLayout();
    QPushButton* btnCancel = new QPushButton(tr("Cancel"), this);
    mBtnApply = new QPushButton(tr("Replace"), this);
    boxButtons->addStretch();
    boxButtons->addWidget(mBtnApply);
    boxButtons->addWidget(btnCancel);

    formLayout->addWidget(new QLabel(tr("Find")));
    formLayout->addWidget(mFindText);
    formLayout->addWidget(new QLabel(tr("Replace")));
    formLayout->addWidget(mReplaceText);
    formLayout->addItem(boxOptions);
    formLayout->addItem(boxButtons);

    connect(mFindText, SIGNAL(textChanged()), this, SLOT(updateApplyButton()));
    connect(btnCancel, SIGNAL(pressed()), this, SLOT(close()));
    connect(mBtnApply, SIGNAL(pressed()), this, SLOT(findreplace()));
    mBtnApply->setEnabled(false);
}

bool
NMFindReplaceDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == mFindText || obj == mReplaceText)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Tab)
            {
                if (obj == mFindText)
                {
                    mReplaceText->setFocus();
                }
                else
                {
                    mUseOnlySelectedText->setFocus();
                }

                return true;
            }
        }
    }
    return false;
}

void
NMFindReplaceDialog::updateApplyButton()
{
    if (mFindText->toPlainText().isEmpty())
    {
        mBtnApply->setEnabled(false);
    }
    else
    {
        mBtnApply->setEnabled(true);
    }
}

void
NMFindReplaceDialog::findreplace()
{
    if (mTextEdit == 0)
    {
        return;
    }

    QString text;
    QString newtext;
    if (mUseOnlySelectedText->isChecked())
    {
        QTextCursor cur = mTextEdit->textCursor();
        text = cur.selectedText();

        if (mUseRegEx->isChecked())
        {
            QRegularExpression regex(mFindText->toPlainText());
            newtext = text.replace(regex, mReplaceText->toPlainText());
        }
        else
        {
            newtext = text.replace(mFindText->toPlainText(), mReplaceText->toPlainText());
        }

        cur.insertText(newtext);
        mTextEdit->setTextCursor(cur);
    }
    else
    {
        text = mTextEdit->toPlainText();
        if (mUseRegEx->isChecked())
        {
            QRegularExpression regex(mFindText->toPlainText());
            newtext = text.replace(regex, mReplaceText->toPlainText());
        }
        else
        {
            newtext = text.replace(mFindText->toPlainText(), mReplaceText->toPlainText());
        }

        mTextEdit->setText(newtext);
    }
}
