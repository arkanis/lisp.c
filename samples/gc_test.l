(define foo nil)

(define looper (lambda (n)
	(if (= n 0)
		nil
		(begin
			(set! foo '(1 2 3 4 5 6 7 8 9 0))
			(looper (- n 1))
		)
	)
))

(print (gc_heap_size))
(looper 1000000)
(print (gc_heap_size))