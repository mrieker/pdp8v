
module DFFModule (U, Q, _Q, T, D, _PC, _PS, _SC, ga, gb, gc, gd, ge, gf);
    input   U;
    output  Q;
    output _Q;
    input   T;
    input   D;
    input _PC;
    input _PS;
    input _SC;

    wire x = D & _SC;

    /*
    // pulsed and-or-invert style latch
    //   addhz=17000 ./raspictl -zynqlib -randmem -mintimes -quiet -cpuhz 29000  =>  avghz 26131
    reg[8:0] count;
    reg pulse;
    always @(posedge U) begin
        if (~ T) begin
            count <= 440;   // tPLH+tPHL = long enough to soak through both nor gates
            pulse <= 0;
        end else if (count != 0) begin
            count <= count - 1;
            pulse <= 1;
        end else begin
            pulse <= 0;
        end
    end

    wire _x;
    StandCell xnot (U, _x, x);
    StandCell snor (U, _Q, _PC & pulse &  x | _PC &  Q);
    StandCell rnor (U,  Q, _PS & pulse & _x | _PS & _Q);
    */

    ///*
    // edge-triggered using 6 nand gate standcells
    output ga, gb, gc, gd, ge, gf;

    assign _Q = ge;
    assign  Q = gf;

    StandCell anand (U, ga, x  & gb & _PC);
    StandCell bnand (U, gb, ga & T  & gc);
    StandCell cnand (U, gc, T  & gd & _PC);
    StandCell dnand (U, gd, gc & ga & _PS);
    StandCell enand (U, ge, gb & gf & _PC);
    StandCell fnand (U, gf, gc & ge & _PS);
    //*/

    /*
    reg lastD, lastT, Q, _Q;

    always @(posedge U) begin
        if (~ _PC | ~ _PS) begin
             Q <= ~ _PS;
            _Q <= ~ _PC;
        end else if (~ lastT & T) begin
             Q <=   lastD;
            _Q <= ~ lastD;
        end
        lastD <= x;
        lastT <= T;
    end
    */
endmodule
