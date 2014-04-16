0 ddrc !c 255 portc !c
0 ddrd !c 255 portd !c

variable curlayout

: inc dup @ 1 + ! ;

: scanbit
    1 and if
        curlayout @ send
    then ;

: scanbyte
    8 0 do dup scanbit
        >> curlayout inc loop ;

: m begin
        layout curlayout !
        pinc @c not scanbyte
    again ; m
