/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2013 Landcare Research New Zealand Ltd
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
 * lumassengine.h
 *
 *  Created on: 22/06/2013
 *      Author: alex
 */

#ifndef LUMASSENGINE_H_
#define LUMASSENGINE_H_

#include <QString>

// DECLARATIONS
void doModel(const QString& modelFile);
void doMOSO(const QString& losFileName);
void doMOSObatch(const QString& losFileName);
void doMOSOsingle(const QString& losFileName);
//void doMOSOrun(const QString& dsfileName,
//		const QString& losSettingsFileName,
//		const QString& perturbItem,
//		float level,
//		int startIdx, int numruns);


bool isFileAccessible(const QString& fileName);

#endif /* LUMASSENGINE_H_ */
