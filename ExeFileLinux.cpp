#include "StdAfx.h"

#if defined(__linux__)

#include "ExeFile.h"
#include "Crashpad.h"
#include <CcpCrash.h>

#include <cerrno>
#include <cstdio>
#include <sched.h>

#if CCP_MEMORY_REPLACE_OPERATOR_NEW

void* operator new( size_t size )
{
	extern const char* g_moduleName;
	return CCP_MALLOC( g_moduleName, size );
}
void* operator new[]( size_t size )
{
	extern const char* g_moduleName;
	return CCP_MALLOC( g_moduleName, size );
}
void operator delete( void* p ) noexcept
{
	CCP_FREE( p );
}
void operator delete[]( void* p ) noexcept
{
	CCP_FREE( p );
}

#endif

void ShowConsoleWindow( ConsoleMode )
{
	// This host is linked as a console executable. Linux has no separate console-allocation step.
}

void PreStartupTest()
{
#if defined( __x86_64__ ) || defined( __i386__ )
	if( !__builtin_cpu_supports( "sse2" ) )
	{
		fprintf( stderr, "SSE2 support is needed to run this program.\n" );
	}
#endif
}

void SetProcessAffinity( int affinity )
{
	cpu_set_t set;
	CPU_ZERO( &set );
	unsigned int mask = static_cast<unsigned int>( affinity );
	for( int cpu = 0; cpu < 31; ++cpu )
	{
		if( mask & ( 1u << cpu ) )
		{
			CPU_SET( cpu, &set );
		}
	}
	if( sched_setaffinity( 0, sizeof( set ), &set ) == 0 )
	{
		CCP_LOG( "Process affinity mask is %d", affinity );
	}
	else
	{
		CCP_LOGERR( "Cannot set affinity mask, mask=%d, err=%d", affinity, errno );
	}
}

bool IsSupportedOSVersion()
{
	// Windows requires Windows 10 or newer. macOS requires 10.15 or newer.
	// The existing source has no Linux minimum-version table.
	return true;
}

#ifndef FE8_EXEFILE_HOST_TEST
int main( int argc, char* argv[] )
{
	CommandLine commandLine = ParseCommandLine( argc, argv );
	return Main( commandLine, IsSupportedOSVersion() );
}
#endif

#endif
