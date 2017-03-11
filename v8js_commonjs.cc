/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | http://www.opensource.org/licenses/mit-license.php  MIT License      |
  +----------------------------------------------------------------------+
  | Author: Jani Taskinen <jani.taskinen@iki.fi>                         |
  | Author: Patrick Reilly <preilly@php.net>                             |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_v8js_macros.h"

extern "C" {
#include "php.h"
#include "zend_exceptions.h"
}

static void v8js_commonjs_split_terms(const char *identifier, std::vector<char *> &terms)
{
    char *term = (char *) emalloc(PATH_MAX), *ptr = term;

    while (*identifier > 0) {
        if (*identifier == '/') {
            if (ptr > term) {
                // Terminate term string and add to terms vector
                *ptr++ = 0;
                terms.push_back(estrdup(term));

                // Reset term string
                ptr = term;
            }
        } else {
            *ptr++ = *identifier;
        }

        identifier++;
    }

    if (ptr > term) {
        // Terminate term string and add to terms vector
        *ptr++ = 0;
        terms.push_back(estrdup(term));
    }

    efree(term);
}

void v8js_commonjs_normalise_identifier(const char *base, const char *identifier, char *normalised_path, char *module_name)
{
    std::vector<char *> id_terms, terms;
    v8js_commonjs_split_terms(identifier, id_terms);

    // If we have a relative module identifier then include the base terms
    if (!strcmp(id_terms.front(), ".") || !strcmp(id_terms.front(), "..")) {
        v8js_commonjs_split_terms(base, terms);
    }

    terms.insert(terms.end(), id_terms.begin(), id_terms.end());

    std::vector<char *> normalised_terms;

    for (std::vector<char *>::iterator it = terms.begin(); it != terms.end(); it++) {
        char *term = *it;

        if (!strcmp(term, "..")) {
            // Ignore parent term (..) if it's the first normalised term
            if (normalised_terms.size() > 0) {
                // Remove the parent normalized term (and free it)
                efree(normalised_terms.back());
                normalised_terms.pop_back();
            }

            // free the ".." term
            efree(term);
        } else if (strcmp(term, ".")) {
            // Add the term if it's not the current term (.)
            normalised_terms.push_back(term);
        } else {
            // Discard "." term
            efree(term);
        }
    }

    // Initialise the normalised path string
    *normalised_path = 0;
    *module_name = 0;

    strcat(module_name, normalised_terms.back());

    efree(normalised_terms.back());
    normalised_terms.pop_back();

    for (std::vector<char *>::iterator it = normalised_terms.begin(); it != normalised_terms.end(); it++) {
        char *term = *it;

        if (strlen(normalised_path) > 0) {
            strcat(normalised_path, "/");
        }

        strcat(normalised_path, term);
        efree(term);
    }
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
