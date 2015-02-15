// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CHALLENGEX64_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CHALLENGEX64_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef CHALLENGEX64_EXPORTS
#define CHALLENGEX64_API __declspec(dllexport)
#else
#define CHALLENGEX64_API __declspec(dllimport)
#endif

// This class is exported from the challenge.x64.dll
class CHALLENGEX64_API Cchallengex64 {
public:
	Cchallengex64(void);
	// TODO: add your methods here.
};

extern CHALLENGEX64_API int nchallengex64;

CHALLENGEX64_API int fnchallengex64(void);
