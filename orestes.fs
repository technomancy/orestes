( 0 ddrc !c 255 portc !c )
( 0 ddrd !c 255 portd !c )

: inc dup @ 1 + swap ! ;

4 22 7 8 10 11 13 14 ( row 1 )

variable row1 8 1 - cells allot

variable rowoffset 0 rowoffset !

: scanbit 1 and if
        layout dup numout @ numout then ;

: scanbyte
    numout
    8 0 do dup scanbit
        >> rowoffset inc loop ;

1 scanbit exit

: m begin
        0 row-offset !
        pind @c not scanbyte
    again ;
