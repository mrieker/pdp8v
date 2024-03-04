
module StandCell (U, _Q, D);
    input   U;
    output _Q;
    input   D;

///*
    reg _Q;
    reg[9:0] counter;

    always @(posedge U) begin
        if (D) begin
            // tPHL = 0.30uS (30 * 10nS)
            if (counter > 29) begin
                counter <= 29;
            end else if (counter != 0) begin
                counter <= counter - 1;
            end else begin
                _Q <= 0;
            end
        end else begin
            // tPLH = 3.0uS (300 * 10nS)
            if (counter > 299) begin
                _Q <= 1;
            end else begin
                counter <= counter + 1;
            end
        end
    end
//*/

endmodule
