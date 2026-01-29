#ifndef DUCTA_UTILS_ERROR_HPP
#define DUCTA_UTILS_ERROR_HPP


namespace ducta {
namespace Utils {

struct Error
{
};

template <class OutputStream>
OutputStream& operator<<(OutputStream& os, const Error& error)
{
    os << "Utils::Error";
    return os;
}


} // namespace Utils
} // namespace ducta

#endif // DUCTA_UTILS_ERROR_HPP