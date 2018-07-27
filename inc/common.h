#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <sys/stat.h>

using namespace std;
typedef std::uint64_t hash_t;

const static hash_t prime = 0x100000001B3ull;
const static hash_t basis = 0xCBF29CE484222325ull;

void ToLowCase(char &c);

void ToUpperCase(char &c);

void ToLowCase(string &str);

void ToUpperCase(string &str);

///
/// \brief 去除文件的后缀名
/// \param 原始文件名
///
/// \remark 转换后的文件名是小写的
///
void NoExtention(string &fileName);

constexpr hash_t get_str_hash(char const* str, hash_t last_value = basis)
{
  return *str ? get_str_hash(str+1, (*str ^ last_value) * prime) : last_value;
}

constexpr hash_t operator "" _hash(char const* p, size_t)
{
  return get_str_hash(p);
}


#endif // COMMON_H
