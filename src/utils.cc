#include "utils.hpp"

#include "fiber.hpp"

#include <unistd.h>

namespace furina{

static thread_local size_t t_thisThreadId = (size_t)-1;

size_t getThreadId(){
    if(unlikely(t_thisThreadId == (size_t)-1)){
        t_thisThreadId = syscall(SYS_gettid);
    }
    return t_thisThreadId;
}

size_t getFiberId(){
    if(getExecFiber() == nullptr){
        return (size_t)-1;
    }else{
        return getExecFiber()->getId();
    }
}

};