0 ddrc !c 255 portc !c
0 ddrd !c 255 portd !c

: inc dup @ 1 + ! ;

: scanbit
    1 and if
        4 send
    then ;

1 scanbit reset

variable curlayout

: scanbyte
    sendint
    8 0 do dup scanbit
        >> curlayout inc loop ;

: m begin
        layout curlayout !
        pind @c not scanbyte
    again ;
