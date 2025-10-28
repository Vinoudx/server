#ifndef __MACRO__
#define __MACRO__

#include <assert.h>

#include "logger.hpp"
#include "utils.hpp"

#define ASSERT(x) do{\
                         if(!(x)){\
                            LOG_ERROR << "Assertion: " << #x ;\
                            LOG_FLUSH;\
                            assert(false);\
                        }\
                        }while(0);

#define ASSERT2(x, w) do{\
                            if(!(x)){\
                                LOG_ERROR << "Assertion: " << #x \
                                << ' '\
                                << w; \
                                LOG_FLUSH;\
                                assert(false);\
                            }\
                            }while(0);
#endif