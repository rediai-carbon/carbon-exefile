#include "StdAfx.h"

const char* g_moduleName = "ExeFileLinuxHostBehavior";

#include "CommandArguments.h"
#include "ExeFile.h"
#include <CcpCrash.h>
#include "Crashpad.h"
#include "FileSystem.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sched.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
int g_failures = 0;

void Check( bool condition, const char* text )
{
	if( !condition )
	{
		std::cerr << "FAIL " << text << '\n';
		++g_failures;
	}
}

std::wstring Widen( const std::string& text )
{
	std::wstring wide;
	wide.reserve( text.size() );
	for( unsigned char character : text )
	{
		wide.push_back( static_cast<wchar_t>( character ) );
	}
	return wide;
}
}

int main()
{
	char temporary[] = "/tmp/fe8-exefile-XXXXXX";
	if( !mkdtemp( temporary ) )
	{
		std::cerr << "FAIL mkdtemp\n";
		return 1;
	}
	if( setenv( "XDG_DATA_HOME", temporary, 1 ) != 0 || setenv( "HOME", temporary, 1 ) != 0 )
	{
		std::cerr << "FAIL setenv\n";
		return 1;
	}

	const std::string appData = GetAppdataFolder();
	Check( appData == temporary, "GetAppdataFolder uses XDG_DATA_HOME" );

	const std::string nested = appData + "/nested/child";
	Check( CreateDirectoryRec( Widen( nested ).c_str() ), "CreateDirectoryRec creates a nested directory" );
	struct stat directory = {};
	Check( stat( nested.c_str(), &directory ) == 0 && S_ISDIR( directory.st_mode ), "nested directory exists" );

	const std::wstring logs = GetLogsFolder();
	const std::wstring suffix = L"/CCP/EVE";
	Check( logs.size() >= suffix.size() && logs.compare( logs.size() - suffix.size(), suffix.size(), suffix ) == 0, "GetLogsFolder keeps the CCP/EVE suffix" );

	char arg0[] = "exefile";
	char arg1[] = "/inherit";
	char arg2[] = "/buildflavor=internal";
	char arg3[] = "/py";
	char arg4[] = "/path:cache=/tmp/cache";
	char arg5[] = "/aflock=1";
	char arg6[] = "/nocrashreportupload";
	char* argv[] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6 };
	CommandLine commandLine = ParseCommandLine( 7, argv );
	CommandArguments arguments = GetCommandArguments( commandLine );
	Check( arguments.consoleMode == console_mode_inherit, "/inherit selects inherit mode" );
	Check( arguments.buildFlavor == L"internal", "/buildflavor=internal is preserved" );
	Check( arguments.affinity == 1, "/aflock=1 is preserved" );
	Check( !arguments.uploadMinidump, "/nocrashreportupload disables upload" );
	char* bareArgv[] = { arg0 };
	CommandArguments bareArguments = GetCommandArguments( ParseCommandLine( 1, bareArgv ) );
	Check( !bareArguments.uploadMinidump, "Linux leaves crash uploads disabled by default" );
	bool foundPath = false;
	for( const std::wstring& path : arguments.searchPaths )
	{
		if( path == L"cache=/tmp/cache" )
		{
			foundPath = true;
		}
	}
	Check( foundPath, "/path entries are preserved" );

	SetProcessAffinity( 1 );
	cpu_set_t actual;
	CPU_ZERO( &actual );
	if( sched_getaffinity( 0, sizeof( actual ), &actual ) == 0 )
	{
		Check( CPU_ISSET( 0, &actual ), "affinity bit 0 remains available" );
	}

	CrashpadCrashInterface* reporter = GetCrashReporter();
	Check( reporter != nullptr, "release build provides a crash reporter" );
	char selfPath[4096] = {};
	ssize_t selfLength = readlink( "/proc/self/exe", selfPath, sizeof( selfPath ) - 1 );
	std::string handlerPath;
	if( selfLength > 0 )
	{
		selfPath[selfLength] = '\0';
		std::string self( selfPath );
		auto slash = self.find_last_of( '/' );
		if( slash != std::string::npos )
		{
			handlerPath = self.substr( 0, slash ) + "/eve_crashmon";
		}
	}
	Check( !handlerPath.empty() && access( handlerPath.c_str(), X_OK ) == 0, "eve_crashmon is staged beside the test" );
	if( reporter && !handlerPath.empty() && access( handlerPath.c_str(), X_OK ) == 0 )
	{
		reporter->EnableCrashReporting( false );
		signal( SIGALRM, []( int ) { _exit( 86 ); } );
		alarm( 10 );
		bool started = reporter->InitializeCrashpad();
		alarm( 0 );
		Check( started, "eve_crashmon starts with uploads disabled" );
		Check( !reporter->IsCrashReportingEnabled(), "uploads stay disabled" );
	}

	if( g_failures != 0 )
	{
		std::cerr << g_failures << " checks failed\n";
		return 1;
	}
	std::cout << "EXEFILE_LINUX_HOST_BEHAVIOR_OK\n";
	return 0;
}
