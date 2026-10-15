/* REGTEST: Regina on CMS */
parse version v; say "Hello from" v
parse source s; say s
say "2**100 =" 2**100
numeric digits 40; say "1/7 =" 1/7
address command "QUERY TIME"
say "rc" rc
address command "STATE RT REX L"
say "STATE rc" rc
address command "LISTFILE RT REX L"
say "LISTFILE rc" rc
address command "QUERY DISK L"
say "QUERY DISK rc" rc
"CP QUERY USERID"
say "rc" rc
call lineout "REGTEST DATA A", "a line"
call lineout "REGTEST DATA A"
say linein("REGTEST DATA A")
