(define fman car)
(define fexp (lambda (a) (car (cdr a))))
(define fmake (lambda (man exp) (list man exp)))
(define fzero (fmake 0 0))
(define ften (fmake 10 0))
(define maxint  8388607)
(define nexttotopbit 4194304)
(define halfmaxint 4194303)
(define minint -8388608)
(define halfminint -4194304)
(define topbit -8388608)
(define fmaxexp (fmake maxint maxint))
(define fminexp (fmake      1 minint))

(define fadd (lambda (a b) (fadd-mx (fman a) (fexp a) (fman b) (fexp b))))
(define fsub (lambda (a b) (fsub-mx (fman a) (fexp a) (fman b) (fexp b))))
(define fmul (lambda (a b) (fmul-mx (fman a) (fexp a) (fman b) (fexp b))))
(define fdiv (lambda (a b) (fdiv-mx (fman a) (fexp a) (fman b) (fexp b))))

; floating point subtraction
(define fsub-mx
    (lambda (aman aexp bman bexp)
        (fadd-mx aman aexp (- bman) bexp)))

; floating point addition
(define fadd-mx
    (lambda (aman aexp bman bexp)
        (cond
            ; if a is zero, result is b
            ((= aman) (fmake bman bexp))
            ; if b is zero, result is a
            ((= bman) (fmake aman aexp))
            ; if aexp > bexp, make either aexp smaller or bexp larger
            ((> aexp bexp)
                (if (fpman-can-be-doubled aman aexp)
                    (fadd-mx (<< aman 1) (- aexp 1) bman bexp)
                    (fadd-mx aman aexp (>> bman 1) (+ bexp 1))))
            ; if aexp < bexp, make either bexp smaller or aexp larger
            ((< aexp bexp)
                (if (fpman-can-be-doubled bman bexp)
                    (fadd-mx aman aexp (<< bman 1) (- bexp 1))
                    (fadd-mx (>> aman 1) (+ aexp 1) bman bexp)))
            ; exponents equal, add aman,bman and use same exponent
            (#T
                (define sman (+ aman bman))
                (define sexp aexp)
                ; if adding overflowed, divide by 2 and use incremented exponent
                ; otherwise, use direct result obtained
                (if (or (and (> aman) (> bman) (< sman)) (and (< aman) (< bman) (> sman)))
                    (fmake (+ (>> sman 1) topbit) (+ sexp 1))
                    (fmake sman sexp))))))

; see if the given mantissa can be doubled and the exponent decremented without overflowing
(define fpman-can-be-doubled
    (lambda (man exp) (and (> exp minint) (<= man halfmaxint) (> man halfminint))))

; floating point multiplication
(define fmul-mx
    (lambda (aman aexp bman bexp)
        ; result is zero if either mantissa is zero
        (if (or (= aman) (= bman)) fzero
            (begin
                ; result is neg iff mantissa signs are different
                (define neg (<> (int (< aman)) (int (< bman))))
                ; get absolute value of mantissas
                (define amanabs (if (< aman) (- aman) aman))
                (define bmanabs (if (< bman) (- bman) bman))
                ; multiply mantissas
                (define pmanhi (umulhi amanabs bmanabs))
                (define pmanlo (umullo amanabs bmanabs))
                ; add the exponents
                (define pexp (+ aexp bexp))
                (cond
                    ; infinity if summation overflowed
                    ((and (> aexp) (> bexp) (< pexp)) fmaxexp)
                    ((and (< aexp) (< bexp) (> pexp)) fminexp)
                    ; normalize product
                    (#T (fnorm48 neg pmanhi pmanlo pexp)))))))

; floating point divide
(define fdiv-mx
    (lambda (aman aexp bman bexp)
        (cond
            ((< aman) (fneg (fdiv-mx (- aman) aexp bman bexp)))
            ((< bman) (fneg (fdiv-mx aman aexp (- bman) bexp)))
            ((= bman) fmaxexp)
            ((= aman) fzero)
            ((= (& aman nexttotopbit)) (fdiv-mx (<< aman 1) (- aexp 1) bman bexp))
            ((= (& bman 1)) (fdiv-mx aman aexp (>> bman 1) (+ bexp 1)))
            (#T
                (define amanlz (- (num-leading-zeroes aman) 1))
                (define aman (<< aman amanlz))
                (define aexp (- aman amanlz))
                (define bmantz (num-trailing-zeroes bman))
                (define bman (>> bman bmantz))
                (define bexp (+ bman bmantz))
                (display "aman=" aman " bman=" bman #\#J)
                (define q (quo aman bman))
                (define r (rem aman bman))
                (define q+ (int (< r (quo bman 2))))
                (fnorm48 #F 0 (+ q q+) (- aexp bexp))))))

(define num-leading-zeroes
    (lambda (n)
        (if (= n) #F
            (if (< n) 0
                (+ (num-leading-zeroes (* n 2)) 1)))))

(define num-trailing-zeroes
    (lambda (n)
        (if (= n) #F
            (if (<> (& n 1)) 0
                (+ (num-trailing-zeroes (/ n 2)) 1)))))

; floating point negate
(define fneg (lambda (a) (fmake (- (fman a)) (fexp a))))

; normalize floating-point number with 48-bit unsigned mantissa
(define fnorm48
    (lambda (neg pmanhi pmanlo pexp)
        (if (and (= pmanhi) (>= pmanlo))
            (fmake (if neg (- pmanlo) pmanlo) pexp)
            (begin
                (define halfpmanlo (& maxint (>> pmanlo 1)))
                (fnorm48
                    neg
                    (>> pmanhi 1)
                    (if (= (& pmanhi 1)) halfpmanlo (+ halfpmanlo minint))
                    (+ pexp 1))))))

; compare two floating point numbers
(define fcmp-mx
    (lambda (aman aexp bman bexp)
        (cond
            ((= bman) aman)
            ((= aman) (- bman))
            ((and (< aman) (> bman)) -1)
            ((and (> aman) (< bman))  1)
            ((< aman) (fcmp-mx (- bman) bexp (- aman) aexp))
            (#T (fman (fsub-mx aman aexp bman bexp))))))

; format the given man,exp to a string
(define ftostr-mx
    (lambda (man exp)
        (cond
            ((= man) "0")
            ((< man) (string "-" (ftostr-mx (- man) exp)))
            (#T (ftostr-six man exp 0)))))

; format float with decimal exponent
; doesn't actually format until man,exp
;   in range 100,000 <= man * 2**exp <= 1,000,000
;  value: man * 2**exp * 10**iexp
;         man > 0
(define ftostr-six
    (lambda (man exp iexp)
        (cond
            ((< (fcmp-mx man exp 100000 0))
                ; man,exp < 100,000
                ; multiply by 10 and decrement iexp
                (define timesten (fmul-mx man exp 10 0))
                (ftostr-six (fman timesten) (fexp timesten) (- iexp 1)))

            ((> (fcmp-mx man exp 1000000 0))
                ; man,exp > 1,000,000
                ; divide by 10 and increment iexp
                (define overten (fdiv-mx man exp 10 0))
                (ftostr-six (fman overten) (fexp overten) (+ iexp 1)))

            (#T
                ; 100,000 <= man,exp <= 1,000,000
                (ftostr-int (ftointr-mx man exp) iexp)))))

; convert man,exp to integer with rounding
(define ftointr-mx
    (lambda (man exp)
        (if (>= exp)
            ; positive exponent, shift mantissa left by exponent
            (<< man exp)
            ; shift to leave exp -1 then shift one last time with rounding
            (>> (+ (>> man (- -1 exp)) 1) 1))))

; format integer with decimal exponent ival * 10**iexp
(define ftostr-int
    (lambda (ival iexp)

        ; trim trailing zeroes from integer
        (if (= (rem ival 10)) (ftostr-int (quo ival 10) (+ iexp 1))

            ; we now have i * 10**iexp
            ;  1 <= ival <= 999,999
            (begin
                (define digits (string ival))
                (define ndigits (strlen digits))

                ; check for negative exponent, ie, have at least one digit to right of where decimal point goes
                (if (< iexp)

                    ; if decimal point can be spliced in digits somewhere, do that instead of exponent
                    ; only do this if we will leave at least one digit to left of decimal point
                    (if (> (+ ndigits iexp))
                        (string (substr digits 0 iexp) #\. (substr digits iexp))

                        ; decimal point to left of all digits
                        ; if a few zeroes can be spliced on front, do that instead of exponent
                        ; allow this format for up to and including 6 digits after decimal point
                        (begin
                            (define nleadingzeroes (- -1 iexp))
                            (if (<= (+ nleadingzeroes ndigits) 6)
                                (string "0." (substr "000000" 0 nleadingzeroes) digits)

                                ; too many zeroes, use exponent format
                                (string "0." digits #\E (+ iexp ndigits)))))

                    ; decimal point to right of all digits
                    ; if number can be expressed as at most 6-digit integer, do that
                    (if (<= (+ ndigits iexp) 6)
                        (string digits (substr "000000" 0 iexp))

                        ; too many digits, output digits followed by exponent
                        (string digits #\E iexp)))))))

; create floatingpoint number from string
;;;;(define ffromst
;;;;    (lambda (s)
;;;;        (if (= (strlen s)) #F
;;;;            (begin
;;;;                (define c (int (charat 0 s)))
;;;;                (if (<= c 32) (ffromst (substr s 1))
;;;;                    (if (= c 45) (fneg (ffromst (substr s 1)))
;;;;                        (begin
;;;;                            (define epos (strpos "E" s))
;;;;                            (define manstr (if (bool? epos) s (substr s 0 epos)))
;;;;                            (define expint (if (bool? epos) 0 (int (substr s (+ epos 1)))))
;;;;                            (if (bool? expint) #F
;;;;                                (begin
;;;;                                    (define dpos (strpos "." manstr))
;;;;                                    (define manint (int (substr manstr 0 dpos)))
;;;;                                    (define mandecstr (substr manstr (+ dpos 1)))
;;;;                                    (define mandecint (int mandecstr))
;;;;                                    (if (or (bool? manint) (bool? mandecint)) #F
;;;;                                        (begin
;;;;                                            (define manflt (fmake10 manint expint))
;;;;                                            (define mandecflt (fmake10 mandecint (- expint (strlen mandecstr))))
;;;;                                            (fadd manflt mandecflt))))))))))))

; make floatingpoint number from given integer mantissa and power-of-ten integer exponent
;;;;(define fmake10
;;;;    (lambda (man exp10)
;;;;        (if (= man) fzero
;;;;            (if (= exp10) (fmake man 0)
;;;;                (begin
;;;;                    (define fman (fmake man 0))
;;;;                    (if (< exp10) (fmake10-int fman (- exp10) fdiv ften)
;;;;                        (fmake10-int fman exp10 fmul ften)))))))
;;;;(define fmake10-int
;;;;    (lambda (val exp10 fop factor)
;;;;        (if (= exp10) val
;;;;            (if (= (& exp10 1))
;;;;                (fmake10-int val (quo exp10 2) (fmul factor ften))
;;;;                (fmake10-int (fop val factor) (quo exp10 2) (fmul factor ften))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define one.0 (fmake 1 0))
(define zero.5 (fmake 1 -1))
(fadd one.0 zero.5)
(fsub one.0 zero.5)
(fmul (fmake 345 0) (fmake 10 0))
(fdiv (fmake 345 0) (fmake 5 1))

(heaproom-gc) (stackroom)

(display "  result: " (ftostr-mx 1  1) #\#J)
(display "  result: " (ftostr-mx 1 -1) #\#J)
(display "  result: " (ftostr-mx 123 4) #\#J)
(display "  result: " (ftostr-mx 456 -7) #\#J)

