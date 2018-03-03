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
#ifndef NMPARAMHIGHLIGHTER_H
#define NMPARAMHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class NMParamHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    typedef enum {
        NM_PARM_EXP = 0,
        NM_MATH_EXP,
        NM_SCRIPT_EXP,
        NM_SQLITE_EXP
    } NMParamExpressionType;

    NMParamHighlighter(QObject *parent);

    void setExpressionType(NMParamExpressionType type)
        {mExpType = type;}
    void setRegularExpression(const QRegularExpression& regex)
        {mRegEx = regex;}

signals:

public slots:
    void highlightBlock(const QString &text);
    void setDarkColorMode(bool bDark=false);


private:
    NMParamExpressionType mExpType;
    QStringList mSQLiteKeywords;
    QStringList mMuParserKeywords;
    QStringList mKernelScriptKeywords;
    QRegularExpression mRegEx;
    QRegularExpression mRegExNum;

    QTextCharFormat normalFormat;
    QTextCharFormat compFormat;
    QTextCharFormat propFormat;
    QTextCharFormat keywordFormat;
    QTextCharFormat idxFormat;
    QTextCharFormat numberFormat;

};

#endif // NMPARAMHIGHLIGHTER_H
