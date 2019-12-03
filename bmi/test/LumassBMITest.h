/******************************************************************************
 * Created by Alexander Herzig
 * Copyright 2019 Landcare Research New Zealand Ltd
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

#if defined _WIN32
	#define WINCALL __stdcall
	#include <windows.h>
	#include <strsafe.h>
	#include <libloaderapi.h>
#else
	#define WINCALL
	#include <dlfcn.h>
#endif

#include <string>

using Logger = void(*)(int, const char*);

// some function pointer type definitions we'll need later on
using _bmi_init                 = int (WINCALL *)(const char*);
using _bmi_update               = int (WINCALL *)(double);
using _bmi_finalize             = int (WINCALL *)(void);
using _bmi_get_start_time       = void (WINCALL *)(double);
using _bmi_get_end_time         = void (WINCALL *)(double);
using _bmi_get_current_time     = void (WINCALL *)(double);
using _bmi_get_time_step        = void (WINCALL *)(double);
using _bmi_set_logger			= void (WINCALL *)(Logger);

// declare bmi function pointers
_bmi_init              bmi_init             ;
_bmi_update            bmi_update           ;
_bmi_finalize          bmi_finalize         ;
_bmi_get_start_time    bmi_get_start_time   ;
_bmi_get_end_time      bmi_get_end_time     ;
_bmi_get_current_time  bmi_get_current_time ;
_bmi_get_time_step     bmi_get_time_step    ;
_bmi_set_logger        bmi_set_logger       ;

// pointer to lumass BMI library
void* lumassbmi = nullptr;

// function declarations
int BMIInit(const char* libpath);

void log(int i, const char* msg);

#if defined _WIN32
std::string GetErrorMsg(void);
#endif