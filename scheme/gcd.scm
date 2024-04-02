
(define gcd
    (lambda (a b)
        (if (= b) a (gcd b (rem a b)))))

