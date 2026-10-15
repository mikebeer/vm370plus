"""9.39: PARSE SOURCE as CMS has it -- 'CMS COMMAND fn ft fm name CMS'
(the mode is '*': where CMS found it is not known here).  Applied to
interprt.c by mkregina.sh."""
import sys
p = sys.argv[1]
s = open(p).read()
old = '''                  Str_catstrTSD( source, " " );
                  Str_catTSD( source, inpfile );
                  break;'''
new = '''                  Str_catstrTSD( source, " " );
#ifdef __CMS__
                  {
                     /* "./BANG.EXEC" -> "BANG EXEC * BANG CMS" */
                     char fn[9], ft[9], buf[48];
                     int i = 0, k, n = Str_len( inpfile );
                     const char *v = inpfile->value;
                     if ( n >= 2 && v[0] == '.' && v[1] == '/' ) { v += 2; n -= 2; }
                     for ( k = 0; i < n && v[i] != '.' && v[i] != ' ' && k < 8; i++ ) fn[k++] = (char)toupper( (unsigned char)v[i] );
                     fn[k] = 0;
                     while ( i < n && ( v[i] == '.' || v[i] == ' ' ) ) i++;
                     for ( k = 0; i < n && v[i] != '.' && v[i] != ' ' && k < 8; i++ ) ft[k++] = (char)toupper( (unsigned char)v[i] );
                     ft[k] = 0;
                     sprintf( buf, "%s %s * %s CMS", fn, ft[0] ? ft : "EXEC", fn );
                     source = Str_makeTSD( Str_len( source ) + strlen( buf ) + 1 );
                     source->len = 0;
                     Str_catstrTSD( source, stype );
                     Str_catstrTSD( source, " " );
                     Str_catstrTSD( source, invo_strings[TSD->systeminfo->invoked] );
                     Str_catstrTSD( source, " " );
                     Str_catstrTSD( source, buf );
                  }
#else
                  Str_catTSD( source, inpfile );
#endif
                  break;'''
assert old in s
s = s.replace(old, new, 1)
if '#include <ctype.h>' not in s:
    s = '#include <ctype.h>\n' + s
open(p, 'w').write(s)
