#include <string>
#include <string_view>


namespace cipher {
using ustring = std::basic_string<unsigned char>;
using ustring_view = std::basic_string_view<unsigned char>;

ustring fromString(std::string_view s);
std::string encodeBase64(ustring_view input);
ustring decodeBase64(std::string_view input);
ustring encryptAES(std::string_view input, ustring_view key);
std::string decryptAES(ustring_view input, ustring_view key);
}