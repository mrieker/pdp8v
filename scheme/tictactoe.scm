
; board<01:00> = cell 1  11 = available
;      <03:02> = cell 2  01 = taken by X
;      <05:04> = cell 3  10 = taken by O
;      <07:06> = cell 4  00 = not used
;      <09:08> = cell 5
;      <11:10> = cell 6
;      <13:12> = cell 7
;      <15:14> = cell 8
;      <17:16> = cell 9

;  cells:
;    1 2 3
;    4 5 6
;    7 8 9

(define XX 1)
(define OO 2)

; top-level function to play a game
; returns char who won: X O 0=tie
(define play-game
    (lambda ()
        (begin
            (define winner (who-won? 0777777))
            (cond
                ((= winner) (display "tie game#J") #\0)
                (#T (define winch (cellch winner 0)) (display winch " wins#J") winch)))))

; see who won a game given a board
; if the board is incomplete, prompt for moves and try again until resolved
; return XX, OO, 0=tie
(define who-won?
    (lambda (board)
        (begin
            (display-board board 1)
            (define winner (compute-winner board))
            (cond
                ((<> winner) winner)
                ((board-full? board) 0)
                (#T (who-won? (get-move (if (< (num-OXs OO board) (num-OXs XX board)) OO XX) board)))))))

; see who if anyone has won the game given a board as is
; return XX, OO, 0=nobody
(define compute-winner
    (lambda (board)
        (cond
            ((OX-won? XX board) XX)
            ((OX-won? OO board) OO)
            (#T 0))))

; see if all cells of a board are occupied
; return #T #F
(define board-full?
    (lambda (board)
        (if (= board) #T
            (and (<> (& board 3) 3) (board-full? (>> board 2))))))

; get number of OOs or XXs in the whole board
; return int
(define num-OXs
    (lambda (OX board)
        (if (= board) 0
            (+ (if (= OX (& 3 board)) 1 0) (num-OXs OX (>> board 2))))))

; prompt for and get a move from the keyboard
; apply it to the given old board
; return new board
(define get-move
    (lambda (OX oldboard)
        (begin
            (define move1 (int (prompt (cellch OX 0) " - enter cell number 1..9 or 0 for the computer to play: ")))
            (if (or (bool? move1) (< move1 0) (> move1 9))
                (begin
                    (display "bad move - must be single digit 0..9#J")
                    (display-board oldboard 1)
                    (get-move OX oldboard))
                (begin
                    (define newboard (modify-board OX (if (= move1) (compute-move0 OX oldboard) (- move1 1)) oldboard))
                    (if (= newboard)
                        (begin
                            (display "cell " move1 " ocupied#J")
                            (display-board oldboard 1)
                            (get-move OX oldboard))
                        newboard))))))

; compute move for the given player
;  input:
;   OX = OO or XX
;   board = existing board
;  returns 0-based cell to play
(define compute-move0
    (lambda (OX board)
        (begin
            ; if we have a winning move, take it
            (define winmove (winning-move OX 0 board))
            (if (>= winmove) winmove
                (begin
                    ; if other player has a winning move, block it
                    (define losmove (winning-move (- (+ XX OO) OX) 0 board))
                    (cond
                        ((>= losmove) losmove)
                        ; take center if it's available
                        ((= (& board 001400) 001400) 4)
                        ; take a corner if one's available
                        ((= (& board 0000003) 0000003) 0)
                        ((= (& board 0000060) 0000060) 2)
                        ((= (& board 0030000) 0030000) 6)
                        ((= (& board 0600000) 0600000) 8)
                        ; all that's left are the sides
                        ((= (& board 0000014) 0000014) 1)
                        ((= (& board 0000300) 0000300) 3)
                        ((= (& board 0006000) 0006000) 5)
                        ((= (& board 0140000) 0140000) 7)
                        (#T -1)))))))

; see if there is a move where the given user wins this play
; return -1: none; else: 0-based move number
(define winning-move
    (lambda (OX move0 board)
        (if (> move0 8) -1
            (begin
                (define newboard (modify-board OX move0 board))
                (if (and (<> newboard) (OX-won? OX newboard)) move0
                    (winning-move OX (+ move0 1) board))))))

; see if XX or OO has won the game given a board as is
; return #T #F
(define OX-won?
    (lambda (OX board)
        (or
            (OX-won-row? OX (& 077 board))
            (OX-won-row? OX (& 077 (>> board 6)))
            (OX-won-row? OX (>> board 12))
            (OX-won-col? OX 0  0 board)
            (OX-won-col? OX 1  0 board)
            (OX-won-col? OX 2  0 board)
            (OX-won-col? OX 0  1 board)
            (OX-won-col? OX 2 -1 board))))

; see if XX or OO has won the given row
; return #T #F
(define OX-won-row?
    (lambda (OX row)
        (if (= row) #T
            (and (= OX (& 3 row)) (OX-won-row? OX (>> row 2))))))

; see if XX or OO has won the skewed column of a board
;  input:
;   OX  = XX or OO
;   col = column (0..2) in top row to check
;   inc = column increment from first row to second row
;  output:
;   return #T #F
(define OX-won-col?
    (lambda (OX col inc board)
        (if (= board) #T
            (and (= (& (>> board (<< col 1)) 3) OX) (OX-won-col? OX (+ col inc) inc (>> board 6))))))

; display the whole board as 3 rows
;  input:
;   board = 18 bits of the board
;   cellnum = 1-based number of first cell of board
(define display-board
    (lambda (board cellnum)
        (if (= board) #F
            (begin
                (display-row (& 077 board) cellnum)
                (display-board (>> board 6) (+ cellnum 3))))))

; display a row of the board
;  input:
;   row = 6 bits of a row of the board
;   cellnum = 1-based number of first cell of row
(define display-row
    (lambda (row cellnum)
        (if (= row) (display #\#J)
            (begin
                (display #\  (cellch (& 3 row) cellnum))
                (display-row (>> row 2) (+ cellnum 1))))))

; return char X O 1..9
(define cellch
    (lambda (cell num)
        (cond
            ((= XX cell) #\X)
            ((= OO cell) #\O)
            ((=  3 cell) (char (+ 48 num)))
            (#T #\-))))

; return new board or 0=cell occupied
(define modify-board
    (lambda (OX move0 board)
        (begin
            (define mask (<< 3 (<< move0 1)))
            (if (<> mask (& mask board)) 0
                (- board mask (<< OX (<< move0 1)) 0)))))

(heaproom)

(play-game)

; find the best move
(define best-move
    (lambda (OX board)
        (for 0 9 1
            (lambda (i)
                (begin
                    (define newboard (modify-board OX i board))
                    (if (= newboard) #F


; for a given board, what are chances that OX wins?
; OX has just moved, other player is next
(define score-board
    (lambda (OX board)
        (begin
            (define other (other-player OX))
            (define score 0)
            (define outof 0)
            (for 0 9 1
                (lambda (i)
                    (begin
                        (define mask (<< 3 (<< i 1)))
                        (if (= (& board mask) mask)
                            (begin
                                (set! score (+ score (score-board other (modify-board OX i board))))
                                (set! outof (+ outof 1))))
                        #F)))
            (cond
                ((<> outof) (/ score outof))
                ((OX-won? OX    board)  1)
                ((OX-won? other board) -1)
                (#T 0)))))

(define for
    (lambda start stop step body)
        (if (= start stop) #F
            (begin
                (define ret (body start))
                (if (or (not (bool? ret)) ret) ret (for (+ start step) stop step body))))))

