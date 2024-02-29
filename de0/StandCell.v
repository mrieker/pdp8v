
module StandCell (U, _Q, D);
    input   U;
    output _Q;
    input   D;
    reg    _Q;

/*
    always @ (posedge U) begin
        _Q = ~ D;
    end
*/

    // tphl =  7 U cycles (140nS)
    // tplh = 31 U cycles (620nS)
    reg[2:0] tphl;
    reg[4:0] tplh;

    always @ (posedge U) begin
        if (D) begin
            tplh <= 0;
            if (tphl != 7) begin
                _Q <= 1;
                tphl <= tphl + 1;
            end else begin
                _Q <= 0;
            end
        end else begin
            tphl <= 0;
            if (tplh != 31) begin
                _Q <= 0;
                tplh <= tplh + 1;
            end else begin
                _Q <= 1;
            end
        end
    end
endmodule
