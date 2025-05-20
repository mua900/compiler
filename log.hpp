#pragma once

#include "token.hpp"
#include <cstdarg>
#include <cstdio>

// @todo take our own string instead of raw char pointers

void errorf(int line, char const * const fmsg, ...);

void error(int line, char const * const msg);
void error(Token t, char const * const msg);
void warning(int line, char const * const msg);
void warningf(int line, char const * const fmsg, ...);


void report_info(char const * const, ...);

void error_token(const Token& token, char const*const msg);
void error_tokenf(const Token& token, char const*const fmsg, ...);

void error(char const * const msg);

void log(char const * const msg);

