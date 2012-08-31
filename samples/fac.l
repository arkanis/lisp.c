#!/home/steven/projects/lisp.c/lisp
; Calculate factorial, nice test for recursion (but does not require lexical scoping or capturing on bytecode level)
(define fac (lambda (n)
	(if (= n 1)
		1
		(* n (fac (- n 1)))
	)
))

; Be aware: (fac 21) will lead to an overflow in the int64_t datatype used for numbers
(print (fac 20))