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
#include "nmlog.h"
#include "NMAddColumnDialog.h"
#include "ui_NMAddColumnDialog.h"

//#include "vtkType.h"

#include <QVariant>

NMAddColumnDialog::NMAddColumnDialog(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
}

NMAddColumnDialog::~NMAddColumnDialog()
{
}

QString NMAddColumnDialog::getColumnName(void)
{
	return ui.lineEditColName->text();
}

int NMAddColumnDialog::getDataType(void)
{
	int ret = -1;
	switch(ui.cbxDataType->currentIndex())
	{
	case 0: //INTEGER
		ret = QVariant::Int;
		break;
	case 1: //REAL
		ret = QVariant::Double;
		break;
	case 2: //STRING
		ret = QVariant::String;
		break;
	default:
		break;
	}

	return ret;
}
