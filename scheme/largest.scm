
(define largest
    (lambda (list)
        (if (null? list)
            #F
            (largest-of (car list) (cdr list)))))

(define largest-of
    (lambda (sofar list)
        (if (null? list) sofar
            (largest-of (larger-of (car list) sofar) (cdr list)))))

(define larger-of (lambda (a b) (if (> a b) a b)))

