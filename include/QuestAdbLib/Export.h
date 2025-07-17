#pragma once

#ifdef _WIN32
    #ifdef QUESTADBLIB_EXPORTS
        #define QUESTADBLIB_API __declspec(dllexport)
    #else
        #define QUESTADBLIB_API __declspec(dllimport)
    #endif
#else
    #define QUESTADBLIB_API
#endif