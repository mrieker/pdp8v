
(define add-x-factory
    (lambda (x)
        (lambda (i) (+ i x))))

(define add-1 (add-x-factory 1))
(define add-2 (add-x-factory 2))

