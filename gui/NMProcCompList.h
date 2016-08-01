 /****************************************************************************** 
 * Created by Alexander Herzig 
 * Copyright 2010,2011,2012 Landcare Research New Zealand Ltd 
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
/*
 * NMProcCompList.h
 *
 *  Created on: 19/06/2012
 *      Author: alex
 */

#ifndef NMPROCCOMPLIST_H_
#define NMPROCCOMPLIST_H_

#include <string>
#include <iostream>

#include <qlistwidget.h>
#include <QMouseEvent>
#include <QChildEvent>
#include <QActionEvent>
#include <QPoint>
#include <QWidget>


#include "nmlog.h"

class NMProcCompList: public QListWidget
{
	Q_OBJECT

public:
	NMProcCompList(QWidget* parent=0);
	virtual ~NMProcCompList();


private:
	std::string ctx;
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);

	QPoint mDragStart;

};

#endif /* NMPROCCOMPLIST_H_ */
