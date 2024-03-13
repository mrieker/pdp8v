
module DLatModule (U, Q, _Q, D, G, _PC, _PS);
    input   U;
    output  Q;
    output _Q;
    input   D;
    input   G;
    input _PC;
    input _PS;

    //reg Q, _Q;

    wire a, b, c, d;

    StandCell anand (U, a, G & D);
    StandCell bnand (U, b, G & a);
    StandCell cnand (U, c, a & d & _PS);
    StandCell dnand (U, d, b & c & _PC);

    assign  Q = c;
    assign _Q = d;

    /*
    always @(*) begin
        if (~ _PC | ~ _PS) begin
             Q <= ~ _PS;
            _Q <= ~ _PC;
        end else if (G) begin
             Q <=   D;
            _Q <= ~ D;
        end
    end
    */
endmodule
