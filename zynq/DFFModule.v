
module DFFModule (U, Q, _Q, T, D, _PC, _PS, _SC);
    input   U;
    output  Q;
    output _Q;
    input   T;
    input   D;
    input _PC;
    input _PS;
    input _SC;

    wire x = D & _SC;

    ///*
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
    //*/

    /*
    // edge-triggered using 6 nand gate standcells
    wire a, b, c, d, e, f;

    assign _Q = e;
    assign  Q = f;

    StandCell anand (U, a, x & b & _PC);
    StandCell bnand (U, b, a & T & c);
    StandCell cnand (U, c, T & d & _PC);
    StandCell dnand (U, d, c & a & _PS);
    StandCell enand (U, e, b & f & _PC);
    StandCell fnand (U, f, c & e & _PS);
    */

    /*
    always @(posedge U) begin
        if (~ _PC | ~ _PS) begin
             Q <= ~ _PS;
            _Q <= ~ _PC;
        end else if (~ lastT & T) begin
             Q <=   lastD;
            _Q <= ~ lastD;
        end
        lastD <= dat;
        lastT <= T;
    end
    */

    /*
    always @(posedge T or negedge _PC or negedge _PS) begin
        if (~ _PC | ~ _PS) begin
             Q <= ~ _PS;
            _Q <= ~ _PC;
        end else if (T) begin
             Q <=   dat;
            _Q <= ~ dat;
        end
    end
    */
endmodule
