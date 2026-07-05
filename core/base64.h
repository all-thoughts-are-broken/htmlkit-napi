#ifndef HTMLKIT_BASE64_H
#define HTMLKIT_BASE64_H

#include <cstddef>
#include <string>
#include <vector>

std::vector<unsigned char> base64_decode(const char* input, size_t length);
std::string base64_encode(const unsigned char* input, size_t length);

#endif // HTMLKIT_BASE64_H
