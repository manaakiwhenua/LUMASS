/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSql module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
 *  original class/file: QSqlCachedResult
 *  adapted 2015 by Alexander Herzig
 *  for compiling together with LUMASS source code
 *
 */

#include "nmqsqlcachedresult_p.h"

#include <qvariant.h>
#include <qdatetime.h>
#include <qvector.h>

QT_BEGIN_NAMESPACE

/*
   NMQSqlCachedResult is a convenience class for databases that only allow
   forward only fetching. It will cache all the results so we can iterate
   backwards over the results again.

   All you need to do is to inherit from NMQSqlCachedResult and reimplement
   gotoNext(). gotoNext() will have a reference to the internal cache and
   will give you an index where you can start filling in your data. Special
   case: If the user actually wants a forward-only query, idx will be -1
   to indicate that we are not interested in the actual values.
*/

static const uint initial_cache_size = 128;

class NMQSqlCachedResultPrivate
{
public:
    NMQSqlCachedResultPrivate();
    bool canSeek(int i) const;
    inline int cacheCount() const;
    void init(int count, bool fo);
    void cleanup();
    int nextIndex();
    void revertLast();

    NMQSqlCachedResult::ValueCache cache;
    int rowCacheEnd;
    int colCount;
    bool forwardOnly;
    bool atEnd;
};

NMQSqlCachedResultPrivate::NMQSqlCachedResultPrivate():
    rowCacheEnd(0), colCount(0), forwardOnly(false), atEnd(false)
{
}

void NMQSqlCachedResultPrivate::cleanup()
{
    cache.clear();
    forwardOnly = false;
    atEnd = false;
    colCount = 0;
    rowCacheEnd = 0;
}

void NMQSqlCachedResultPrivate::init(int count, bool fo)
{
    Q_ASSERT(count);
    cleanup();
    forwardOnly = fo;
    colCount = count;
    if (fo) {
        cache.resize(count);
        rowCacheEnd = count;
    } else {
        cache.resize(initial_cache_size * count);
    }
}

int NMQSqlCachedResultPrivate::nextIndex()
{
    if (forwardOnly)
        return 0;
    int newIdx = rowCacheEnd;
    if (newIdx + colCount > cache.size())
        cache.resize(qMin(cache.size() * 2, cache.size() + 10000));
    rowCacheEnd += colCount;

    return newIdx;
}

bool NMQSqlCachedResultPrivate::canSeek(int i) const
{
    if (forwardOnly || i < 0)
        return false;
    return rowCacheEnd >= (i + 1) * colCount;
}

void NMQSqlCachedResultPrivate::revertLast()
{
    if (forwardOnly)
        return;
    rowCacheEnd -= colCount;
}

inline int NMQSqlCachedResultPrivate::cacheCount() const
{
    Q_ASSERT(!forwardOnly);
    Q_ASSERT(colCount);
    return rowCacheEnd / colCount;
}

//////////////

NMQSqlCachedResult::NMQSqlCachedResult(const QSqlDriver * db): QSqlResult (db)
{
    d = new NMQSqlCachedResultPrivate();
}

NMQSqlCachedResult::~NMQSqlCachedResult()
{
    delete d;
}

void NMQSqlCachedResult::init(int colCount)
{
    d->init(colCount, isForwardOnly());
}

bool NMQSqlCachedResult::fetch(int i)
{
    if ((!isActive()) || (i < 0))
        return false;
    if (at() == i)
        return true;
    if (d->forwardOnly) {
        // speed hack - do not copy values if not needed
        if (at() > i || at() == QSql::AfterLastRow)
            return false;
        while(at() < i - 1) {
            if (!gotoNext(d->cache, -1))
                return false;
            setAt(at() + 1);
        }
        if (!gotoNext(d->cache, 0))
            return false;
        setAt(at() + 1);
        return true;
    }
    if (d->canSeek(i)) {
        setAt(i);
        return true;
    }
    if (d->rowCacheEnd > 0)
        setAt(d->cacheCount());
    while (at() < i + 1) {
        if (!cacheNext()) {
            if (d->canSeek(i))
                break;
            return false;
        }
    }
    setAt(i);

    return true;
}

bool NMQSqlCachedResult::fetchNext()
{
    if (d->canSeek(at() + 1)) {
        setAt(at() + 1);
        return true;
    }
    return cacheNext();
}

bool NMQSqlCachedResult::fetchPrevious()
{
    return fetch(at() - 1);
}

bool NMQSqlCachedResult::fetchFirst()
{
    if (d->forwardOnly && at() != QSql::BeforeFirstRow) {
        return false;
    }
    if (d->canSeek(0)) {
        setAt(0);
        return true;
    }
    return cacheNext();
}

bool NMQSqlCachedResult::fetchLast()
{
    if (d->atEnd) {
        if (d->forwardOnly)
            return false;
        else
            return fetch(d->cacheCount() - 1);
    }

    int i = at();
    while (fetchNext())
        ++i; /* brute force */
    if (d->forwardOnly && at() == QSql::AfterLastRow) {
        setAt(i);
        return true;
    } else {
        return fetch(i);
    }
}

QVariant NMQSqlCachedResult::data(int i)
{
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i >= d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return QVariant();

    return d->cache.at(idx);
}

bool NMQSqlCachedResult::isNull(int i)
{
    int idx = d->forwardOnly ? i : at() * d->colCount + i;
    if (i >= d->colCount || i < 0 || at() < 0 || idx >= d->rowCacheEnd)
        return true;

    return d->cache.at(idx).isNull();
}

void NMQSqlCachedResult::cleanup()
{
    setAt(QSql::BeforeFirstRow);
    setActive(false);
    d->cleanup();
}

void NMQSqlCachedResult::clearValues()
{
    setAt(QSql::BeforeFirstRow);
    d->rowCacheEnd = 0;
    d->atEnd = false;
}

bool NMQSqlCachedResult::cacheNext()
{
    if (d->atEnd)
        return false;

    if(isForwardOnly()) {
        d->cache.clear();
        d->cache.resize(d->colCount);
    }

    if (!gotoNext(d->cache, d->nextIndex())) {
        d->revertLast();
        d->atEnd = true;
        return false;
    }
    setAt(at() + 1);
    return true;
}

int NMQSqlCachedResult::colCount() const
{
    return d->colCount;
}

NMQSqlCachedResult::ValueCache &NMQSqlCachedResult::cache()
{
    return d->cache;
}

void NMQSqlCachedResult::virtual_hook(int id, void *data)
{
    QSqlResult::virtual_hook(id, data);
}

void NMQSqlCachedResult::detachFromResultSet()
{
    cleanup();
}

void NMQSqlCachedResult::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy policy)
{
    QSqlResult::setNumericalPrecisionPolicy(policy);
    cleanup();
}


QT_END_NAMESPACE
