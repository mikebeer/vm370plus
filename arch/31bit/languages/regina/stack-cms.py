"""9.39: Regina's unnamed queue is the CMS console stack on CMS (PUSH,
QUEUE, QUEUED(); PULL already reads it through stdin).  Applied to
stack.c by mkregina.sh."""
import sys
p = sys.argv[1]
s = open(p).read()
decl = '''#ifdef __CMS__
int cms_attn(const char *line, int len, int lifo);
int cms_queued(void);
#endif
'''
anchor = 'int stack_lifo( tsd_t *TSD, streng *line, const streng *queue_name )\n{'
assert anchor in s
s = s.replace(anchor, decl + anchor, 1)
for fn, lifo in (('stack_lifo', 1), ('stack_fifo', 0)):
    a = 'int %s( tsd_t *TSD, streng *line, const streng *queue_name )\n{' % fn
    i = s.index(a)
    j = s.index('   assert( line != NULL ) ;', i)
    k = j + len('   assert( line != NULL ) ;')
    s = s[:k] + '''
#ifdef __CMS__
   if ( !queue_name )
   {
      cms_attn( line->value, line->len, %d ) ;
      Free_stringTSD( line ) ;
      return 0 ;
   }
#endif''' % lifo + s[k:]
a = 'int lines_in_stack( tsd_t *TSD, const streng *queue_name )\n{'
i = s.index(a)
j = s.index('   st = (stk_tsd_t *)TSD->stk_tsd;', i)
s = s[:j] + '''#ifdef __CMS__
   if ( !queue_name )
      return cms_queued() + ((stk_tsd_t *)TSD->stk_tsd)->current_queue->u.i.elements ;
#endif
''' + s[j:]
open(p, 'w').write(s)
