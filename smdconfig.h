
//TODO: detect platform and define the following
//#define SMD_PLATFORM_WINDOWS
//#define SMD_PLATFORM_MACOS
//#define SMD_PLATFORM_ etc, etc

#if defined (PLATFORM_MACOS)
#  define SMD_PLATFORM_USE_POSIX
#endif

#if defined(_WIN32)
#  if defined(libsmd_EXPORTS)
#    define SMD_API __declspec(dllexport)
#  else
#    define SMD_API __declspec(dllimport)
#  endif
#else // non windows
#  define SMD_API
#endif

