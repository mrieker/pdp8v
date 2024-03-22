; print hello in infinite loop
; run with slow tty so can see the lights roll in HLT instruction
(define printloop (lambda () (begin (display " hello hello hello hello hello hello ") (printloop))))
(printloop)
