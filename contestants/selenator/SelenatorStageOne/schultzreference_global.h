#ifndef SCHULTZSOFTWARESOLUTIONS_GLOBAL_H
#define SCHULTZSOFTWARESOLUTIONS_GLOBAL_H

#if defined(SCHULTZSOFTWARESOLUTIONS_LIBRARY)
#  define S3_EXPORT __declspec(dllexport)
#else
#  define S3_EXPORT __declspec(dllimport)
#endif

#endif // SCHULTZSOFTWARESOLUTIONS_GLOBAL_H
