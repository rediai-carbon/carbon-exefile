#include "StdAfx.h"
#if defined(__linux__)

#include "FileSystem.h"

#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

namespace
{
constexpr mode_t kDirectoryMode = 0777;
}

std::string GetAppdataFolder()
{
	const char* xdg = std::getenv( "XDG_DATA_HOME" );
	if( xdg && xdg[0] != '\0' )
	{
		return xdg;
	}
	const char* home = std::getenv( "HOME" );
	if( !home || home[0] == '\0' )
	{
		return {};
	}
	return std::string( home ) + "/.local/share";
}

bool CreateDirectoryRec( const wchar_t* dir )
{
	if( !mkdir( CW2A( dir ), kDirectoryMode ) )
	{
		return true;
	}
	if( errno == EEXIST )
	{
		return true;
	}
	std::wstring full( dir );
	std::wstring::size_type separator = full.find_last_of( L"/\\" );
	if( separator != std::wstring::npos )
	{
		std::wstring parent = full.substr( 0, separator );
		if( !CreateDirectoryRec( parent.c_str() ) )
		{
			return false;
		}
		return !mkdir( CW2A( dir ), kDirectoryMode );
	}
	return false;
}

const std::wstring& GetLogsFolder()
{
	static std::wstring result;
	if( result.empty() )
	{
		std::wstring appDataFolder = (const wchar_t*)CA2W( GetAppdataFolder().c_str() );
		if( !appDataFolder.empty() )
		{
			result = appDataFolder + L"/CCP/EVE";
		}
	}
	return result;
}

#endif
