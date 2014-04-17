( 0 ddrc !c 255 portc !c )
( 0 ddrd !c 255 portd !c )
1 debug !

: inc dup @ 1 + swap ! ;

4 22 7 8 10 11 13 14 ( row 1 )

8 allot row1 

variable rowoffset 0 rowoffset !

: scanbit 1 and if
        row1 rowoffset cells + @ numout then ;

: scanbyte
    numout
    8 0 do dup scanbit
        >> rowoffset inc loop ;

1 scanbit exit

: m begin
        0 row-offset !
        pind @c not scanbyte
    again ;
