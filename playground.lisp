#lang racket

(define nil null)

(define (append l1 l2)
  (if (eq? l1 nil)
    l2
    (cons (first l1) (append (rest l1) l2))))

(define (select pred list)
  (if (eq? nil list)
      list
      (if (pred (first list))
          (cons (first list) (select pred (rest list)))
          (select pred (rest list)))))

(define (collect worker list)
  (if (eq? list nil)
      list
      (cons (worker (first list)) (collect worker (rest list)))))

(define (quicksort list)
  (if (eq? list nil)
      list
      (let ( (pivot (first list)) )
        (let (
              (left (select (lambda (n) (< n pivot)) list))
              (right (select (lambda (n) (> n pivot)) list))
              )
          (append (quicksort left) (cons pivot right))
          ))))

(quicksort '(5 8 1 9 2))