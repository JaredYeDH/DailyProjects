#lang racket

(define (make-queue)
  (mcons empty empty)
  )
(define (front-ptr q)
  (mcar q)
  )
(define (back-ptr q)
  (mcdr q)
  )
(define (set-front-ptr! q v)
  (set-mcar! q v)
  )
(define (set-back-ptr! q v)
  (set-mcdr! q v)
  )

(define (empty-queue? q)
  (null? (front-ptr q))
  )
(define (front-queue q)
  (mcar (front-ptr q))
  )
(define (insert-queue! q v)
  (let ((newn (mcons v empty)))
    (if (empty-queue? q)
      (begin (set-front-ptr! q newn)
             (set-back-ptr! q newn))
      (begin (set-mcdr! (back-ptr q) newn)
             (set-back-ptr! q newn)))
    q)
  )
(define (delete-queue! q)
  (if (empty-queue? q)
    (error "Queue is empty!")
    (set-front-ptr! q (mcdr (front-ptr q))))
  q
  )
(define (print-queue q)
  (display (front-ptr q))
  (newline)
  )

(define q1 (make-queue))
(print-queue (insert-queue! q1 'a))
(print-queue (insert-queue! q1 'b))
(print-queue (delete-queue! q1))
(print-queue (delete-queue! q1))