255 ddrb !c
0 ddrd !c 255 portd !c
0 ddrf !c 255 portf !c

4 constant rows
11 constant columns

19 18 12  24  28  0  23 21 8  26 20 columns allot row0
51 15 14  13  11  0  10 9  7  22 4  columns allot row1
29 27 6   25  5  101 17 16 54 55 56 columns allot row2
41 43 108 102 42 104 44 0  52 47 40 columns allot row3

variable rowoffset 0 rowoffset !
variable currentrow row0 currentrow !

row3 row2 row1 row0 rows allot layer1

variable currentlayer layer1 currentlayer !

variable pressedcount 0 pressedcount !

: pressinsert
    dup pressedcount @ 7 < if if ( logical and would be nice here )
            pressedkeys pressedcount @ cells + ! ( not getting set?? )
            pressedcount @ 1 + pressedcount !
    then then ;

: pressmodifier
    dup numout
    100 - dup pressedmodifiers @ or pressedmodifiers ! ;

: press ( keycode -- ! )
    dup 100 > if pressmodifier
    else pressinsert then ;

: scanbit 1 and if ( shifted-byte -- ! )
        currentrow @ i cells + @ press then ;

: scanbyte ( portbyte -- ! )
    0 do dup scanbit 1 >> loop ;

: scanrow ( -- )
    pind @c not 8 scanbyte drop
    pinf @c not 4 >> columns 8 - scanbyte drop ;

: clearpressed ( -- )
    6 0 do 0 pressedkeys i cells + ! loop
    0 pressedmodifiers !
    0 pressedcount ! ;

: setrow ( rownum -- )
    dup
    1 swap << not portb !c ( set output pin low for this row )
    cells currentlayer @ + @ currentrow ! ;

: scan ( -- )
    rows 0 do
        i setrow scanrow
        onboard not if 255 pinc !c ( -- just for testing ) then
    loop
    usbsend
    clearpressed ;

: main begin scan again ;

scan usbsend