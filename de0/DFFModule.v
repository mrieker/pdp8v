
module DFFModule (U, Q, _Q, T, D, _PC, _PS, _SC);
    input   U;
    output  Q;
    output _Q;
    input   T;
    input   D;
    input _PC;
    input _PS;
    input _SC;

    wire a, b, c, d, e, f;

    assign  Q = e;
    assign _Q = f;

    StandCell AA (U, a, b & d & _PS);
    StandCell BB (U, b, a & T & _PC);
    StandCell CC (U, c, d & T & b);
    StandCell DD (U, d, c & D & _PC & _SC);
    StandCell EE (U, e, f & b & _PS);
    StandCell FF (U, f, e & c & _PC);
endmodule
