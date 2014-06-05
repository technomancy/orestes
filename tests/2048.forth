0 0 0 0
0 0 0 0
0 0 0 0
0 0 0 0 16 allot board

0 0 0 0
0 0 0 0
0 0 0 0
0 0 0 0 16 allot temp

0 1 2 3 0 1 2 3 8 allot moves

variable move moves move !

: 4@ ( loc -- w x y z )
    4 0 do
        1 cells + dup @ swap
    loop drop ;

: 4! ( w x y z target -- )
    4 0 do
        over over ! swap drop
        1 cells +
    loop drop ;

: 2dup ( x y -- x y x y )
    dup 3 pick swap ;

: pushback ( w x y z -- z w x y )
    4 roll 4 roll 4 roll ;

: 2swap ( w x y z -- y z w x )
    pushback pushback ;

: insert ( -- )
    10 rand if 2 else 4 then
    begin 16 rand cells board + dup @
        if drop again else ! then ;

: display ( -- )
    4 0 do
        4 0 do
            board j i 4 * + cells + @ numout
        loop
        cr
    loop cr ;

: read ( -- n )
    move @ @
    move @ 1 cells + move ! ;

: >board ( -- )
    16 0 do
        temp i cells + @
        board i cells + !
    loop ;

: rotate ( -- )
    4 0 do
        4 0 do
            board j 4 * i + cells + @
            temp i 4 * j + cells + !
        loop
    loop
    >board ;

: collapse
    3 pick not if rot drop 0 pushback then
    2 pick not if swap drop 0 pushback then
    dup not if drop 0 pushback then ;

: addfront 2dup = if + 0 pushback then ;

: add ( w x y z -- neww newx newy newz )
    addfront
    pushback addfront 4 roll
    2swap addfront ;

: applyrow ( row -- )
    board i 4 * cells + dup
    4@ collapse add 5 roll 4! ;

: apply ( move -- )
    ( TODO: extra rotations to work around 0 0 do noop loop bug )
    4 + dup 0 do rotate loop
    4 0 do applyrow loop
    4 - 0 do rotate loop ;

: main insert display read apply ;

: reset 4 0 do 0 0 0 0 board i 4 * + 4! loop ;

main main main main main main main main exit