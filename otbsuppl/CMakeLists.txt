#******************************************************************************
# Created by Alexander Herzig
# Copyright 2025-2026 New Zealand Institute for Bioeconomy Science Limited
# 
# This file is part of 'LUMASS', which is free software: you can redistribute
# it and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#*******************************************************************************

PROJECT(otbsuppl)

if(RASSUPPORT)
   ADD_SUBDIRECTORY(rimageio ${otbsuppl_BINARY_DIR}/rimageio)
endif()

#ADD_SUBDIRECTORY(GDALRATImageIO ${otbsuppl_BINARY_DIR}/GDALRATImageIO)
#ADD_SUBDIRECTORY(NETCDFIO ${otbsuppl_BINARY_DIR}/NETCDFIO)
add_subdirectory(core ${otbsuppl_BINARY_DIR}/core)
ADD_SUBDIRECTORY(filters ${otbsuppl_BINARY_DIR}/filters)
