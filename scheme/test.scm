(define zz (lambda (x) (+ x 1)))
(zz 23)

(display "initial heaproom " (heaproom) " gc " (heaproom-gc) "#J")

(define sum (lambda va (sum-list va)))
(define sum-list (lambda (lis) (if (null? lis) 0 (+ (car lis) (sum-list (cdr lis))))))
(sum)
(sum 10)
(sum 10 20)
(sum 10 20 30)
(sum 10 20 30 40)

(define divtest
    (lambda (dividend divisor)
        (begin
            (define q (quo dividend divisor))
            (define r (rem dividend divisor))
            (display "  " dividend " / " divisor " = "
                q " r " r " = " (+ (* q divisor) r) "#J"))))
(divtest  11  4)
(divtest  11 -4)
(divtest -11  4)
(divtest -11 -4)
(divtest  11000  400)
(divtest  11000 -400)
(divtest -11000  400)
(divtest -11000 -400)
(divtest  1100000  40000)
(divtest  1100000 -40000)
(divtest -1100000  40000)
(divtest -1100000 -40000)
(divtest  1100000  7)
(divtest  1100000 -7)
(divtest -1100000  7)
(divtest -1100000 -7)

(display 1 2 "#J" 4 5 "#J")

(define is-even? (lambda (n) (or  (=  n) (is-odd?  ((if (< n) + -) n 1)))))
(define is-odd?  (lambda (n) (and (<> n) (is-even? ((if (< n) + -) n 1)))))
(define eotestit
    (lambda (nums)
        (begin
            (if (null? nums)
                (display "  eotest done#J")
                (begin
                    (define n (car nums))
                    (display "  eotest " n "  even? " (is-even? n) "  odd? " (is-odd? n) "#J")
                    (eotestit (cdr nums)))))))
(eotestit (list 12 23 -12 -23 34 56))
(display "even/odd test done#J")

#f #t empty
(define alist (cons 1 2 3))
(car alist)
(cdr alist)
(null? alist)
(null? empty)
(cons -45 -6789 -7654321 alist)
(list 4 5 6 alist)
(length alist)

(define abc 123)
"abc=" abc
(define add3 (lambda (a b c) (+ a b c))) 
"add3=" 
(add3 34 56 78) (if #t 2 3) (if #f 4 5)

"seven ="
(+ 3 4)

1 123 4567 890123 (list "abcd" "efg") (list "" "Q" "Qw" "Qwe" (list "Qwer" "Qwert" "Qwerty" "Qwerty#J#"##U") "QwertyUI" "QwertyUIO" "QwertyUIOP")

(display 1 2 3 "abcdefghijk" #t -234)

(display "minimal heaproom " (heaproom) " gc " (heaproom-gc) "#J")

(define divtest #f)
(define is-even? #f)
(define is-odd? #f)
(define eotestit #f)
(define alist #f)
(define abc #f)
(define add3 #f)

(display "final heaproom " (heaproom) " gc " (heaproom-gc) "#J")

(display "done#J")
