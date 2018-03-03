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
#include <QDebug>
#include "NMParamHighlighter.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

#include "NMModelController.h"
#include "NMParamEdit.h"

NMParamHighlighter::NMParamHighlighter(QObject* parent)
    : QSyntaxHighlighter(parent), mExpType(NM_PARM_EXP)
{
                    // aggregate functions
    mSQLiteKeywords << "avg" << "count" << "group_concat" << "max"
                    << "min" << "sum" << "total";
                    // date and time functions
    mSQLiteKeywords << "date" << "time" << "datetime" << "julianday"
                    << "strftime";
                    // keywords
    mSQLiteKeywords << "ABORT" << "ACTION" << "ADD" << "AFTER" << "ALL"
                    << "ALTER" << "ANALYZE" << "AND" << "AS" << "ASC"
                    << "ATTACH" << "AUTOINCREMENT" << "BEFORE" << "BEGIN"
                    << "BETWEEN" << "BY" << "BLOB" << "CASCADE" << "CASE" << "CAST"
                    << "CHECK" << "COLLATE" << "COLUMN" << "COMMIT" << "CONFLICT"
                    << "CONSTRAINT" << "CREATE" << "CROSS" << "CURRENT_DATE"
                    << "CURRENT_TIME" << "CURRENT_TIMESTAMP" << "DATABASE"
                    << "DEFAULT" << "DEFERRABLE" << "DFERRED" << "DELETE" << "DESC"
                    << "DETACH" << "DISTINCT" << "DROP" << "EACH" << "ELSE"
                    << "END" << "ESCAPE" << "EXCEPT" << "EXCLUSIVE" << "EXISTS"
                    << "EXPLAIN" << "FAIL" << "FOR" << "FOREIGN" << "FROM"
                    << "FULL" << "GLOB" << "GROUP" << "HAVING" << "IF" << "IGNORE"
                    << "IMMEDIATE" << "IN" << "INDEX" << "INDEXED" << "INITIALLY"
                    << "INNER" << "INSERT" << "INSTEAD" << "INTEGER" << "INTERSECT" << "INTO"
                    << "IS" << "ISNULL" << "JOIN" << "KEY" << "LEFT" << "LIKE";
    mSQLiteKeywords << "MATCH" << "NATURAL" << "NO" << "NOT" << "NOTNULL" << "NULL"
                    << "OF" << "OFFSET" << "ON" << "OR" << "ORDER" << "OUTER"
                    << "PLAN" << "PRAGMA" << "PRIMARY" << "QUERY" << "RAISE"
                    << "REAL" << "RECURSIVE"
                    << "REFERENCES" << "REGEXP" << "REINDEX" << "RELEASE"
                    << "RENAME" << "REPLACE" << "RESTRICT" << "RIGHT" << "ROLLBACK"
                    << "ROW" << "SAVEPOINT" << "SELECT" << "SET" << "TABLE"
                    << "TEMP" << "TEMPORARY" << "TEXT" << "THEN" << "TO" << "TRANSACTION"
                    << "TRIGGER" << "UNION" << "UNIQUE" << "UPDATE" << "USING"
                    << "VACUUM" << "VALUES" << "VIEW" << "VIRTUAL" << "WHEN" << "WHERE" << "WITH";
                    // spatialite functions
    mSQLiteKeywords << "CastToInteger" << "CastToDouble" << "CastToText" << "CastToBlob"
                    << "ForceAsNull" << "MD5Checksum" << "MD5TotalChecksum";
                    // SQL math functions
    mSQLiteKeywords << "Abs" << "Acos" << "Asin" << "Atan" << "Ceil" << "Ceiling"
                    << "Cos" << "Cot" << "Degrees" << "Exp" << "Floor" << "Ln" << "Log"
                    << "Log2" << "Log10" << "PI" << "Pow" << "Power" << "Radians"
                    << "Sign" << "Sin" << "Sqrt" << "Stddev_pop" << "Stddev_samp"
                    << "Tan" << "Var_pop" << "Var_samp";


    mMuParserKeywords << "sin" << "cos" << "tan" << "asin" << "acos" << "atan" << "sinh"
                      << "cosh" << "fmod" << "tanh" << "asinh" << "acosh" << "atanh" << "log2"
                      << "log10" << "ln" << "exp" << "sqrt" << "sign" << "rint" << "rand"
                      << "abs" << "min" << "max" << "sum" << "avg";

    mKernelScriptKeywords = mMuParserKeywords;
    mKernelScriptKeywords << "for" << "numPix" << "centrePixIdx" << "addr"
                          << "thid" << "kwinVal" << "tabVal" << "neigDist";

//    mRegEx = QRegularExpression("((?<open>\\$\\[)*"
//                                "(?(<open>)|\\b)"
//                                "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
//                                "(?<sep1>(?(<open>):|(?>__)))*"
//                                "(?<prop>(?(<sep1>)\\g<comp>))*"
//                                "(?<sep2>(?(<prop>)(?(<open>)):))*"
//                                "(?<idx>(?(<sep2>)[0-9]+)))");

    mRegEx = QRegularExpression("((?<open>\\$\\[)*"
                                "(?(<open>)|\\b)"
                                "(?<comp>[a-zA-Z]+(?>[a-zA-Z0-9]|_(?!_))*)"
                                "(?<sep1>(?(<open>):|(?>__)))*"
                                "(?<prop>(?(<sep1>)\\g<comp>))*"
                                "(?<sep2>(?(<prop>)(?(<open>)):))*"
                                "(?(<sep2>)("
                                "(?<numidx>[0-9]+)(?:\\]\\$|\\$\\[)|"
                                "(?<stridx>[^\\r\\n\\$\\[\\]]*))))");

    mRegExNum = QRegularExpression("\\b(\\d+(?<pt>\\.)*(?(pt)\\d)*(?<sn>e)*(?(sn)[+-]\\d+))\\b");
    setDarkColorMode();
}

void
NMParamHighlighter::setDarkColorMode(bool bDark)
{
    if (bDark)
    {
        normalFormat.setForeground((QColor(180,180,180)));

        compFormat.setForeground(QColor(255,120,255));

        propFormat.setForeground(QColor(11,223,84));

        keywordFormat.setForeground(QColor(0,255,255));

        idxFormat.setForeground(QColor(255, 230, 0));

        numberFormat.setForeground(QColor(255,80,80));
    }
    else
    {
        normalFormat.setForeground((Qt::black));

        compFormat.setForeground(Qt::darkMagenta);

        propFormat.setForeground(Qt::darkGreen);

        keywordFormat.setForeground(Qt::darkBlue);

        idxFormat.setForeground(QColor(200, 133, 0));

        numberFormat.setForeground(Qt::darkRed);
    }
}

void
NMParamHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
    {
        return;
    }

    QTextEdit* edit = qobject_cast<QTextEdit*>(this->parent());
    if (edit == 0)
    {
        return;
    }

    // =================================================================
    // looking for numbers e.g. 3.5 or 133 or 1e-9 or 1e+3
    // =================================================================
    QRegularExpressionMatchIterator numit = mRegExNum.globalMatch(text);
    while (numit.hasNext())
    {
        QRegularExpressionMatch match = numit.next();
        if (match.hasMatch())
        {
            setFormat(match.capturedStart(0), match.capturedLength(0), numberFormat);
        }
    }


    // =================================================================================
    // looking for paramter expressions, e.g. imgId__ColumnName or $[component:property:index]$
    // =================================================================================
    QRegularExpressionMatchIterator mit = mRegEx.globalMatch(text);
    while (mit.hasNext())
    {
        QRegularExpressionMatch match = mit.next();

        QStringRef open = match.capturedRef("open");
        QStringRef comp = match.capturedRef("comp");
        QStringRef sep1 = match.capturedRef("sep1");
        QStringRef prop = match.capturedRef("prop");
        //QStringRef idx  = match.capturedRef("idx");
        QStringRef numidx  = match.capturedRef("numidx");
        QStringRef stridx  = match.capturedRef("stridx");


        if (!comp.isEmpty())
        {
            if ((!numidx.isEmpty() || !stridx.isEmpty()) && !sep1.isEmpty() && sep1.compare(QString("__")) != 0)
            {
                if (!numidx.isEmpty())
                {
                    setFormat(numidx.position(), numidx.length(), idxFormat);
                }
                else
                {
                    setFormat(stridx.position(), stridx.length(), idxFormat);
                }
            }

            if (!prop.isEmpty())
            {
                if (  (   sep1.compare(QString("__")) == 0
                       && (mExpType == NM_MATH_EXP || mExpType == NM_SCRIPT_EXP)
                       && numidx.isEmpty()
                       && stridx.isEmpty())
                   || (sep1.compare(QString(":")) == 0)
                       && !open.isEmpty())
                {
                    setFormat(comp.position(), comp.length(), compFormat);
                    setFormat(prop.position(), prop.length(), propFormat);
                }
            }
            else
            {
                QTextCharFormat theFormat = normalFormat;
                switch(mExpType)
                {
                    case NM_SQLITE_EXP:
                    {
                        if (mSQLiteKeywords.contains(comp.toString(), Qt::CaseInsensitive))
                        {
                            theFormat = keywordFormat;
                        }
                        else if (!open.isEmpty())
                        {
                            theFormat = compFormat;
                        }
                    }
                    break;

                    case NM_MATH_EXP:
                    {
                        if (mMuParserKeywords.contains(comp.toString(), Qt::CaseInsensitive))
                        {
                            theFormat = keywordFormat;
                        }
                        else
                        {
                            theFormat = normalFormat;
                        }
                    }
                    break;

                    case NM_SCRIPT_EXP:
                    {
                        if (mKernelScriptKeywords.contains(comp.toString(), Qt::CaseInsensitive))
                        {
                            theFormat = keywordFormat;
                        }
                        else
                        {
                            theFormat = normalFormat;
                        }
                    }
                    break;

                    case NM_PARM_EXP:
                    {
                        if (!open.isEmpty())
                        {
                            theFormat = compFormat;
                        }
                    }
                    break;

                    default:
                        break;
                }
                setFormat(comp.position(), comp.length(), theFormat);
            }
        }
    }
}
