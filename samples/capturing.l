; This sample is based on the test_capturing() test from bytecode_interpreter_test.c
; The interpreter test contains the bytecode for almost identical functions.

; Basic idea: gen_adders is supposed to generate n adder functions. Each adder function
; captures the current n (e.g. 3, 2, 1) on definition time. Therefore each frame needs to
; be captured and the scoping has to work reliably.

(define gen_adders (lambda (n)
	(if (= n 0)
		nil
		(cons
			(lambda (x) (+ x n))
			(gen_adders (- n 1))
		)
	)
))

(define adders (gen_adders 3))
(define add3 (first adders))
(define add2 (first (rest adders)))

; Should print 12 (10 + 3 + 2)
(print (add2 (add3 7)))