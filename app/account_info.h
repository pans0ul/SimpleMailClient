
#ifndef ACCOUNT_INFO_H
#define ACCOUNT_INFO_H

#if __has_include("inc_with_passwd.h")
    #include "inc_with_passwd.h"
#elif __has_include("inc_default.h")
    #include "inc_default.h"
#else
    // Alternative implementation or fallback
    #warning "Optional header not found, using fallback"
#endif

#endif
