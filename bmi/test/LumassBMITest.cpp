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

#include "LumassBMITest.h"

#include <iostream>
#include <sstream>


/* ErrorExit is based on these sources	https://docs.microsoft.com/en-nz/windows/desktop/Debug/retrieving-the-last-error-code 	https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror*/

#ifdef _WIN32
std::string GetErrorMsg(void)
{
	DWORD errorID = GetLastError();
	LPSTR lpMsgBuf;

	size_t size = FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					errorID,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPSTR)&lpMsgBuf,
					0, NULL);

	std::string msg(lpMsgBuf, size);
	std::stringstream sstr;
	sstr << "ERROR " << errorID << ": " << msg << std::endl;
	
	LocalFree(lpMsgBuf);
	return sstr.str();
}

#define GET_LIB_PTR( libpath ) HMODULE lumassbmi = LoadLibraryA((LPCSTR) libpath );
#define GET_FUNC_PTR( funname ) GetProcAddress(lumassbmi, funname)
#define CLOSE_LUMASSBMI FreeLibrary( (HMODULE)lumassbmi );

#else
    #ifdef __linux
        #define GET_LIB_PTR( libpath ) lumassbmi = dlopen( libpath , RTLD_LAZY );
        #define GET_FUNC_PTR( funname ) dlsym(lumassbmi, funname)
        #define CLOSE_LUMASSBMI dlclose( lumassbmi );
    #endif
#endif

int BMIInit(const char* libpath)
{
    GET_LIB_PTR(libpath)

    if (lumassbmi == nullptr)
    {
#ifdef WIN32
        std::string funcmsg = GetErrorMsg();
        std::cout << "Init of " << libpath << " failed with: \n" << funcmsg;
#else
        std::cout << "Init of " << libpath << " failed!";
#endif
        return EXIT_FAILURE;
    }

    bmi_init = reinterpret_cast<_bmi_init>(GET_FUNC_PTR("initialize"));
    if (bmi_init == nullptr)
    {
        std::cout << "_bmi_init not initialised" << std::endl;
        return EXIT_FAILURE;
    }

    bmi_update = reinterpret_cast<_bmi_update>(GET_FUNC_PTR("update"));
    if (bmi_update == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_finalize = reinterpret_cast<_bmi_finalize>(GET_FUNC_PTR("finalize"));
    if (bmi_finalize == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_get_start_time = reinterpret_cast<_bmi_get_start_time>(GET_FUNC_PTR("get_start_time"));
    if (bmi_get_start_time == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_get_end_time = reinterpret_cast<_bmi_get_end_time>(GET_FUNC_PTR("get_end_time"));
    if (bmi_get_end_time == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_get_current_time = reinterpret_cast<_bmi_get_current_time>(GET_FUNC_PTR("get_current_time"));
    if (bmi_get_current_time == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_get_time_step = reinterpret_cast<_bmi_get_time_step>(GET_FUNC_PTR("get_time_step"));
    if (bmi_get_time_step == nullptr)
    {
        return EXIT_FAILURE;
    }

    bmi_set_logger = reinterpret_cast<_bmi_set_logger>(GET_FUNC_PTR("set_logger"));
    if (bmi_set_logger == nullptr)
    {
        return EXIT_FAILURE;
    }

    return 0;
}

void log(int i, const char* msg)
{
	std::stringstream rep;
	const int alevel = i;
	std::string level;
	switch (alevel)
	{
	case 0: level = "ALL"; break;
	case 1: level = "DEBUG"; break;
	case 2: level = "INFO"; break;
	case 3: level = "WARNING"; break;
    case 4: level = "ERROR"; CLOSE_LUMASSBMI; break;
    case 5: level = "FATAL"; CLOSE_LUMASSBMI; break;
	default: level = "NONE";
	}

	std::cout << "--" << level << "-- " << msg << std::endl;
}

/*  test application for the BMI-ENABLED LUMASS ENGINE
 *
 *  @argc :
 *  @argv : 0: prog name; 1: engine lib path; 2: yaml config file path
 *
 *  For simplicity, here we use the error code of '1' to indicate any
 *  failure.
 *
 */


int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: LumassBMITest <LUMASS BMI library path> "
                  << "<yaml configuration file>" << std::endl;
        return EXIT_FAILURE;
    }

    const char* libpath = argv[1];
    const char* config = argv[2];

	std::stringstream msg;
    if (BMIInit(libpath))
    {
        msg << "Initialization of LUMASS BMI interface failed!"
                  << std::endl;
		log(4, msg.str().c_str());
        return EXIT_FAILURE;
    }

	bmi_set_logger(log);

    if (bmi_init(config) != 0)
    {
		msg << "bmi_init failed!" << std::endl;
		log(4, msg.str().c_str());
        return EXIT_FAILURE;
    }

    if (bmi_update(0) != 0)
    {
		msg << "bmi_update failed!" << std::endl;
		log(4, msg.str().c_str());
        return EXIT_FAILURE;
    }

    if (bmi_finalize() != 0)
    {
		msg << "bmi_finalize failed!" << std::endl;
		log(4, msg.str().c_str());
        return EXIT_FAILURE;
    }

    msg << "LumassBMITest successfully completed!" << std::endl;
	log(2, msg.str().c_str());
    CLOSE_LUMASSBMI;
    return EXIT_SUCCESS;
}
