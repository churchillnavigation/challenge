#pragma once

#include <exception>
#include <string>

namespace XC {
    void verify(bool b);
    void verify(bool b, const char* err_str);

    class errorExcept_t : public std::exception {
        std::string err_message;
    public:
        errorExcept_t();
        errorExcept_t(const char* err_str);
        ~errorExcept_t();

        std::string what();
    };
}