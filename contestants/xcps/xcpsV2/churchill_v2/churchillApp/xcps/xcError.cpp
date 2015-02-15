#include "stdafx.h"
#include "xcError.h"

namespace XC {
    void verify(bool b) {
        if (!b) {
            throw errorExcept_t("");
        }
    }

    void verify(bool b, const char* err_str) {
        if (!b) {
            throw errorExcept_t(err_str);
        }
    }

    //-------------------------------------------------------------------
    // class 
    errorExcept_t::errorExcept_t() {
    }

    errorExcept_t::errorExcept_t(const char* err_str) :
        err_message(err_str) {
    }

    errorExcept_t::~errorExcept_t() {
    }

    std::string errorExcept_t::what() {
        return err_message;
    }
} // end of name space XC