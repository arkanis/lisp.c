; recursive function
(define fac (lambda (n)
	(if (= n 0)
		1
		(* n (fac (- n 1)))
	)
))
(fac 4)

; closures
(define make_adder (lambda (a)
	(lambda (b) (+ a b))
))
(define +10 (make_adder 10))