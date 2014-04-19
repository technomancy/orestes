: nestelse 0 if 39 ( nesting 1 )
        0 if ( nesting 2 )
            37 then ( dropping 2 )
        35 else 30 then ;

: ohai if 88 + then ;

: hi if 12 32 + then 16 1 ohai ;

1 hi

nestelse .s