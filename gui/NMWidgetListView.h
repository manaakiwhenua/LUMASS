/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2014 Landcare Research New Zealand Ltd
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

#ifndef NMWIDGETLISTVIEW_H
#define NMWIDGETLISTVIEW_H

#include <QWidget>
#include <QList>
#include <QMap>
#include <QBoxLayout>
#include <QPushButton>

#include <string>

class NMWidgetListView : public QWidget
{
    Q_OBJECT
public:
    explicit NMWidgetListView(QWidget *parent = 0);

signals:

public slots:

    void addWidgetItem(QWidget* widget, const QString& btnLabel);

    void removeWidgetItem(int index);
    void removeWidgetItem(const QString& name);

    void setWidgetItemVisible(int index, bool visible);
    void setWidgetItemVisible(const QString& name, bool visible);

    int getWidgetIndex(const QString& name);
    int getWidgetItemCount(void);
    bool isWidgetItemVisible(int index);
    bool isWidgetItemVisible(const QString& name);

    QWidget* getWidgetItem(int index);
    QWidget* getWidgetItem(const QString& name);

    QString getWidgetItemName(int index);

protected slots:

    void btnPushed(bool);
    void updateWidgets(void);

protected:
    QList<QPushButton*> mBtnList;
    QMap<QPushButton*, QWidget*> mWidgetList;
    QVBoxLayout* mVBoxLayout;

private:
    static const std::string ctx;

};

#endif // NMWIDGETLISTVIEW_H
