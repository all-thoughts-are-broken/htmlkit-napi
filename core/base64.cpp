#include "base64.h"

#include <array>

static constexpr char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::vector<unsigned char> base64_decode(const char* input, size_t length) {
    static constexpr unsigned char invalid = 0xff;
    static constexpr unsigned char padding = 0xfe;
    static const std::array<unsigned char, 256> table = [] {
        std::array<unsigned char, 256> values{};
        values.fill(invalid);
        for (unsigned char i = 0; i < 64; ++i) {
            values[static_cast<unsigned char>(kBase64Alphabet[i])] = i;
        }
        values[static_cast<unsigned char>('=')] = padding;
        values[static_cast<unsigned char>('\r')] = padding;
        values[static_cast<unsigned char>('\n')] = padding;
        values[static_cast<unsigned char>('\t')] = padding;
        values[static_cast<unsigned char>(' ')] = padding;
        return values;
    }();

    std::vector<unsigned char> output;
    output.reserve(length / 4 * 3);

    unsigned int buffer = 0;
    int bits = 0;
    for (size_t i = 0; i < length; ++i) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        const unsigned char value = table[c];
        if (value == invalid) {
            return {};
        }
        if (value == padding) {
            if (input[i] == '=') {
                break;
            }
            continue;
        }
        buffer = (buffer << 6) | value;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            output.push_back(static_cast<unsigned char>((buffer >> bits) & 0xff));
        }
    }

    return output;
}

std::string base64_encode(const unsigned char* input, size_t length) {
    std::string output;
    output.reserve(((length + 2) / 3) * 4);

    for (size_t i = 0; i < length; i += 3) {
        const unsigned int octet_a = input[i];
        const unsigned int octet_b = i + 1 < length ? input[i + 1] : 0;
        const unsigned int octet_c = i + 2 < length ? input[i + 2] : 0;
        const unsigned int triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output.push_back(kBase64Alphabet[(triple >> 18) & 0x3f]);
        output.push_back(kBase64Alphabet[(triple >> 12) & 0x3f]);
        output.push_back(i + 1 < length ? kBase64Alphabet[(triple >> 6) & 0x3f]
                                        : '=');
        output.push_back(i + 2 < length ? kBase64Alphabet[triple & 0x3f] : '=');
    }

    return output;
}
