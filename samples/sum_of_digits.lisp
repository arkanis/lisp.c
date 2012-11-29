(define sod (lambda (n) 
	(if (< n 10)
		n
		(+ (% n 10) (sod (/ n 10)))
	)
))
(print (sod 173846))