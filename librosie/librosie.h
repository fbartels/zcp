#ifndef _LIBROSIE_H
#define _LIBROSIE_H 1

#include <string>
#include <vector>

extern "C" {

bool rosie_clean_html(const std::string &in, std::string *const out, std::vector<std::string> *const errors);

} /* extern "C" */

#endif /* LIBROSIE_H */
