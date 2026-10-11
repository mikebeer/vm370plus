#!/usr/bin/env python3
"""chrpatch.py COBC_DIR -- cobc writes character values as C character
constants ('0', ' ') instead of numbers (48, 32), so that the C it
generates is right when GCC380 compiles it in EBCDIC on CMS (9.19).
Printable ASCII becomes 'c'; other values (hex literals, LOW-/HIGH-VALUE)
stay numbers.  Identical meaning in ASCII.  Every edit must match exactly."""
import os, sys
D = sys.argv[1]
def ed(p, pairs):
    p = os.path.join(D, p)
    t = open(p).read()
    if 'chrval' in t or 'output_char' in t or 'cb_int_chr' in t:
        print('already patched:', p); return
    for a, b, n in pairs:
        c = t.count(a)
        assert c == n, (p, a, c)
        t = t.replace(a, b)
    open(p, 'w').write(t)

ed('tree.h', [
 ('\tint\t\t\tval;\t\t/* Integer value */\n',
  '\tint\t\t\tval;\t\t/* Integer value */\n\tint\t\t\tchrval;\t\t/* a character: output as \'c\' */\n', 1),
 ('extern cb_tree\t\t\tcb_int_hex (const int);',
  'extern cb_tree\t\t\tcb_int_hex (const int);\nextern cb_tree\t\t\tcb_int_chr (const int);', 1)])
ed('tree.c', [('#if CACHED_INTEGERS\ncb_tree\ncb_int (const int n)', '''/* A character value: codegen writes it as a C character constant, so the
   generated program is right in the target's character set (EBCDIC for
   GCC380 on CMS) -- cobc itself runs in ASCII */
cb_tree
cb_int_chr (const int n)
{
	cb_tree	x;
	if (n < 32 || n > 126) {
		return cb_int (n);
	}
	x = cb_int_uncached (n);
	CB_INTEGER (x)->chrval = 1;
	return x;
}

#if CACHED_INTEGERS
cb_tree
cb_int (const int n)''', 1)])

p = os.path.join(D, 'codegen.c')
t = open(p).read()
if 'output_char' not in t:
    i = t.index('\noutput (const char *fmt, ...)')
    j = t.index('\n}\n', i) + 3
    t = t[:j] + '''
/* A character value as a C character constant when it is printable ASCII
   (it came from COBOL text), else as a number (hex literals, LOW/HIGH-
   VALUE): right in ASCII and, compiled on CMS by GCC380, in EBCDIC. */
static void
output_char (const int c)
{
	if (c == '\\'' || c == '\\\\') {
		output ("'\\\\%c'", c);
	} else if (c >= 32 && c <= 126) {
		output ("'%c'", c);
	} else {
		output ("%d", c);
	}
}
''' + t[j:]
    reps = [
     ('''#else
		output ("%d", CB_INTEGER (x)->val);
#endif
		break;''', '''#else
		if (CB_INTEGER (x)->chrval) {
			output_char (CB_INTEGER (x)->val);
		} else {
			output ("%d", CB_INTEGER (x)->val);
		}
#endif
		break;''', 2),
     ('output (") - %d)", *(CB_LITERAL (p->argv[1])->data));',
      'output (") - ");\n\t\t\toutput_char (*(CB_LITERAL (p->argv[1])->data));\n\t\t\toutput (")");', 1),
     ('output (", %d, %d);", value, f->occurs_max);',
      'output (", ");\n\t\toutput_char (value);\n\t\toutput (", %d);", f->occurs_max);', 1),
     ('output (") = %d;", value);', 'output (") = ");\n\t\toutput_char (value);\n\t\toutput (";");', 1),
     ('output (", %d, ", value);', 'output (", ");\n\t\t\toutput_char (value);\n\t\t\toutput (", ");', 1),
     ('output (", %d, %d);", value, f->size);',
      'output (", ");\n\t\t\toutput_char (value);\n\t\t\toutput (", %d);", f->size);', 1),
     ('output (", %d, ", l->data[0]);', 'output (", ");\n\t\t\toutput_char (l->data[0]);\n\t\t\toutput (", ");', 1),
     ('output (", %d, %d);", l->data[0], size);',
      'output (", ");\n\t\t\toutput_char (l->data[0]);\n\t\t\toutput (", %d);", size);', 1),
     ('output (") = %d;", c);', 'output (") = ");\n\t\toutput_char (c);\n\t\toutput (";");', 1),
     ('output (", %d, ", c);', 'output (", ");\n\t\t\toutput_char (c);\n\t\t\toutput (", ");', 3),
     ('output (", %d, %d);", c, size);',
      'output (", ");\n\t\t\t\toutput_char (c);\n\t\t\t\toutput (", %d);", size);', 1),
     ('output (") = %u;", l->data[0]);', 'output (") = ");\n\t\t\toutput_char (l->data[0]);\n\t\t\toutput (";");', 1),
     ('output (", %u, %d);", (unsigned int)buffchar, size);',
      'output (", ");\n\t\t\toutput_char (buffchar);\n\t\t\toutput (", %d);", size);', 1),
     ('output (", %u, %d);", (unsigned int)buffchar,',
      'output (", ");\n\t\t\toutput_char (buffchar);\n\t\t\toutput (", %d);",', 1),
     ('output (" + %d, %u, %d);",\n\t\t\t\toffset, (unsigned int)buffchar, n);',
      'output (" + %d, ", offset);\n\t\t\toutput_char (buffchar);\n\t\t\toutput (", %d);", n);', 1),
     ('''		if (CB_TREE_CLASS (x) != CB_CLASS_NUMERIC) {
			output ("%d", CB_LITERAL (x)->data[0]);''', '''		if (CB_TREE_CLASS (x) != CB_CLASS_NUMERIC) {
			output_char (CB_LITERAL (x)->data[0]);''', 1)]
    for a, b, n in reps:
        c = t.count(a)
        assert c == n, (a, c)
        t = t.replace(a, b)
    open(p, 'w').write(t)
ed('typeck.c', [
 ('return CB_BUILD_FUNCALL_2 ("$E", x, cb_int (c));', 'return CB_BUILD_FUNCALL_2 ("$E", x, cb_int_chr (c));', 1),
 ('\t\t\t\t   cb_int (c), CB_BUILD_CAST_LENGTH (x));', '\t\t\t\t   cb_int_chr (c), CB_BUILD_CAST_LENGTH (x));', 1),
 ("\t\t\t\t\t\tcb_int (' '),", "\t\t\t\t\t\tcb_int_chr (' '),", 3),
 ('cb_int (l->data[0]),', 'cb_int_chr (l->data[0]),', 2),
 ('return CB_BUILD_FUNCALL_2 ("$E", dst, cb_int (bbyte));', 'return CB_BUILD_FUNCALL_2 ("$E", dst, cb_int_chr (bbyte));', 1),
 ('\t\t\t\t\t   cb_int (bbyte),', '\t\t\t\t\t   cb_int_chr (bbyte),', 1)])
print('chrpatch: done')
