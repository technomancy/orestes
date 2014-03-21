( orestes.fs )

4 constant key-rows
11 constant key-columns
key-rows key-columns * constant key-count

5 constant debounces

variable last-matrix-state key-rows 1 - allot
variable matrix-state key-rows 1 - allot
variable debouncing-state key-rows 1 - allot
variable delta-state key-count 1 - allot
variable layer key-count 1 - allot

: last-matrix last-matrix-state + ;
: matrix matrix-state + ;
: debouncing debouncing-state + ;
: delta delta-state + ;



: ndrop 0 do drop loop ;

: ** ( n m -- n^m )
    1 swap  0 ?do over * loop  nip ;



: init ; ( TODO: set up ports for each row and column )

( TODO: will need to set the pin for this row high, read a byte, then go low )
: read-cols { row-num }
    time&date 5 ndrop \ seconds
    row-num +
;

: debounce-row { row debounced }
    row read-cols
    1 ms ( wait for switch to settle )
    dup row debouncing @ = if
        debounced if
            drop row debounced 1 - recurse
        else row matrix ! endif
    else
        row debouncing !
        row debounced recurse endif ;

: read-keys key-rows 0 do
        i debounces debounce-row
    loop ;

: deltas key-rows 0 do
        i last-matrix @
        i matrix @
        xor i delta !
    loop ;

: reset-last key-rows 0 do
        i matrix @ i last-matrix !
    loop ;

: map-keycodes { against } key-rows 0 do
    loop ;

: send
    0 do cr ." pressed: " . loop
    \ 0 do cr ." released: " . loop
;

: main
    init
    begin
        debounces read-keys
        deltas
        \ dup last-matrix map-keycodes ( releases release-count )
        matrix map-keycodes ( releases r-count presses p-count )
        send ( empties the stack )
        reset-last
    again ;