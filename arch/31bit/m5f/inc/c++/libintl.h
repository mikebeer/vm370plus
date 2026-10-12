/* libintl.h for VM/370plus CMS: no message catalogues.  libstdc++'s
   std::messages (GNU locale model) asks for them; the text comes back
   untranslated. */
#ifndef _LIBINTL_H
#define _LIBINTL_H 1
#define gettext(m) ((char *)(m))
#define dgettext(d, m) ((char *)(m))
#define dcgettext(d, m, c) ((char *)(m))
#define ngettext(m1, m2, n) ((char *)((n) == 1 ? (m1) : (m2)))
#define textdomain(d) ((char *)(d))
#define bindtextdomain(d, dir) ((char *)(dir))
#define bind_textdomain_codeset(d, c) ((char *)(c))
#endif
