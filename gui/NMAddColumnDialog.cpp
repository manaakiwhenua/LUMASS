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

#include "vtkType.h"

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
	case 0: //"Unsigned Char":
		ret = VTK_UNSIGNED_CHAR;
		break;
	case 1: //"Signed Char":
		ret = VTK_SIGNED_CHAR;
		break;
	case 2: //"Short":
		ret = VTK_SHORT;
		break;
	case 3: //"Integer":
		ret = VTK_INT;
		break;
	case 4: //"Long":
		ret = VTK_LONG;
		break;
	case 5: //"Float":
		ret = VTK_FLOAT;
		break;
	case 6: //"Double":
		ret = VTK_DOUBLE;
		break;
	case 7: //"String":
		ret = VTK_STRING;
		break;
	default:
		ret = VTK_STRING;
		break;
	}

	return ret;
}
