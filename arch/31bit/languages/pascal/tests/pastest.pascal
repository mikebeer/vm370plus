program pastest ( output ) ;
(* New Stanford Pascal on VM/370plus CMS: records, sets, strings, *)
(* recursion, real arithmetic                                       *)
type person = record
                name : string ( 20 ) ;
                age : integer
              end ;
     charset = set of char ;
var p : array [ 1 .. 3 ] of person ;
    i , total : integer ;
    vowels : charset ;
    s : string ( 40 ) ;
    n : integer ;
    x : real ;

function fact ( k : integer ) : integer ;
begin
  if k <= 1 then fact := 1 else fact := k * fact ( k - 1 )
end ;

begin
  p [ 1 ] . name := 'Ada' ;    p [ 1 ] . age := 36 ;
  p [ 2 ] . name := 'Grace' ;  p [ 2 ] . age := 85 ;
  p [ 3 ] . name := 'Niklaus' ; p [ 3 ] . age := 89 ;
  total := 0 ;
  for i := 1 to 3 do total := total + p [ i ] . age ;
  writeln ( 'PASTEST: ages total ' , total : 1 ) ;
  vowels := [ 'a' , 'e' , 'i' , 'o' , 'u' ] ;
  s := 'stanford pascal on vm/370plus' ;
  n := 0 ;
  for i := 1 to length ( s ) do
    if s [ i ] in vowels then n := n + 1 ;
  writeln ( 'vowels in "' , s , '": ' , n : 1 ) ;
  writeln ( '10! = ' , fact ( 10 ) : 1 ) ;
  x := 0.0 ;
  for i := 1 to 10 do x := x + 1.0 / i ;
  writeln ( 'H(10) = ' , x : 10 : 6 ) ;
  writeln ( 'sqrt(2) = ' , sqrt ( 2.0 ) : 12 : 9 )
end .
