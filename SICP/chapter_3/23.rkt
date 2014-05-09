#lang racket
(require scheme/mpair)

(define (make-node v prev next)
  (mlist v prev next)
  )
(define (value-node n)
  (mcar n)
  )
(define (prev-node n)
  (mcar (mcdr n))
  )
(define (next-node n)
  (mcar (mcdr (mcdr n)))
  )
(define (set-prev-node! n v)
  (set-mcar! (mcdr n) v)
  )
(define (set-next-node! n v)
  (set-mcar! (mcdr (mcdr n)) v)
  )
(define (insert-after-node n v)
  (let ((nextn (next-node n)))
    (let ((newn (make-node v n nextn)))
      (set-prev-node! nextn newn)
      (set-next-node! n newn)
      newn))
  )
(define (remove-node n)
  (let ((prev (prev-node n))(next (next-node n)))
    (set-prev-node! next prev)
    (set-next-node! prev next)
    n)
  )

(define (make-deque)
  (let ((head (make-node empty empty empty)))
    (set-prev-node! head head)
    (set-next-node! head head)
    head)
  )
(define (front-node-deque dq)
  (next-node dq)
  )
(define (back-node-deque dq)
  (prev-node dq)
  )

(define (empty-deque? dq)
  (eq? (front-node-deque dq) (next-node (back-node-deque dq)))
  )
(define (front-value-deque dq)
  (value-node (front-node-deque dq))
  )
(define (back-value-deque dq)
  (value-node (back-node-deque dq))
  )
(define (insert-front-deque! dq v)
  (insert-after-node (prev-node (front-node-deque dq)) v)
  dq
  )
(define (delete-front-deque! dq)
  (if (empty-deque? dq)
    (error "Can't delete!")
    (begin (remove-node (front-node-deque dq))
           dq))
  )
(define (insert-back-deque! dq v)
  (insert-after-node (back-node-deque dq) v)
  dq
  )
(define (delete-back-deque! dq)
  (if (empty-deque? dq)
    (error "Can't delete!")
    (begin (remove-node (back-node-deque dq))
           dq))
  )
(define (deque->list dq) 
  (define (iter node end)
    (if (eq? node end)
      empty
      (cons (value-node node) (iter (next-node node) end)))
    )
  (iter (front-node-deque dq) (next-node (back-node-deque dq)))
  )
(define (print-deque dq)
  (display (deque->list dq))
  (newline)
  )

(define dq (make-deque))
(print-deque (insert-front-deque! dq 2))
(print-deque (insert-back-deque! dq 3))
(print-deque (insert-back-deque! dq 4))
(print-deque (insert-front-deque! dq 1))
(print-deque (delete-front-deque! dq))
(print-deque (delete-back-deque! dq))
(print-deque (delete-back-deque! dq))
(print-deque (delete-back-deque! dq))