
module DLatModule (U, Q, _Q, D, G, _PC, _PS);
    input   U;
    output  Q;
    output _Q;
    input   D;
    input   G;
    input _PC;
    input _PS;

    wire a, b, c, d;

    assign  Q = c;
    assign _Q = d;

    StandCell AA (U, a, D & G);
    StandCell BB (U, b, a & G);
    StandCell CC (U, c, d & a & _PS);
    StandCell DD (U, d, c & b & _PC);
endmodule
